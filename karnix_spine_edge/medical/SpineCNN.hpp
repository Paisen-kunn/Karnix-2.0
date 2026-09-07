/**
 * SpineCNN.hpp  (v2 – dataset training + inference)
 * Karnix Spine Edge – Spine & Bone Early Screening CNN
 *
 * Architecture (channels-first, 1×128×128 grayscale input):
 *   Conv2D(1→8,  k=3,s=1,p=0) → ReLU → MaxPool(2)  → [8×63×63]
 *   Conv2D(8→16, k=3,s=1,p=0) → ReLU → MaxPool(2)  → [16×30×30]
 *   Conv2D(16→32,k=3,s=1,p=0) → ReLU → MaxPool(2)  → [32×14×14]
 *   Flatten → Linear(6272→128) → ReLU → Linear(128→2) → Softmax
 *
 * Output:  index 0 → Normal,  index 1 → Structural Abnormality
 *
 * Training: SGD + CrossEntropy+Softmax combined gradient.
 * Image support: PPM/PGM native; JPEG/PNG/TIFF via macOS sips (offline).
 * C++14 compatible.  No external libraries.
 */

#ifndef SPINE_CNN_HPP
#define SPINE_CNN_HPP

#include "../../neural_network/utils/Tensor.hpp"
#include "../../neural_network/layers/ConvolutionLayer.hpp"
#include "../../neural_network/layers/ActivationFunctions.hpp"
#include "../../neural_network/layers/PoolingLayers.hpp"
#include "../../neural_network/layers/FullyConnectedLayer.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <algorithm>

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Image I/O  (PPM/PGM native; JPEG/PNG via macOS sips – fully offline)       */
/* ─────────────────────────────────────────────────────────────────────────── */
#ifndef KARNIX_SPINE_CLI_RAW_IMAGE_DEFINED
struct RawImage {
    std::vector<double> pixels;  // normalised [0,1], grayscale
    int width  = 0;
    int height = 0;
};
#endif

namespace ImageIO {

    inline void skip_ws(std::istream& is) {
        while (true) {
            int c = is.peek();
            if (c == '#') { std::string ln; std::getline(is, ln); }
            else if (std::isspace(c)) { is.get(); }
            else break;
        }
    }

    inline RawImage load_ppm(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string magic; f >> magic; skip_ws(f);
        int w, h, maxval;
        f >> w >> h; skip_ws(f); f >> maxval; f.get();
        RawImage img; img.width = w; img.height = h;
        img.pixels.resize(h * w);
        if (magic == "P6") {
            std::vector<unsigned char> buf(h*w*3);
            f.read(reinterpret_cast<char*>(buf.data()), buf.size());
            for (int i = 0; i < h*w; ++i) {
                double r = buf[3*i+0]/static_cast<double>(maxval);
                double g = buf[3*i+1]/static_cast<double>(maxval);
                double b = buf[3*i+2]/static_cast<double>(maxval);
                img.pixels[i] = 0.299*r + 0.587*g + 0.114*b;
            }
        } else if (magic == "P5") {
            std::vector<unsigned char> buf(h*w);
            f.read(reinterpret_cast<char*>(buf.data()), buf.size());
            for (int i = 0; i < h*w; ++i)
                img.pixels[i] = buf[i]/static_cast<double>(maxval);
        } else if (magic == "P3") {
            for (int i = 0; i < h*w; ++i) {
                int r,g,b; f >> r >> g >> b;
                img.pixels[i] = (0.299*r+0.587*g+0.114*b)/maxval;
            }
        } else if (magic == "P2") {
            for (int i = 0; i < h*w; ++i) { int v; f>>v; img.pixels[i]=v/static_cast<double>(maxval); }
        } else {
            throw std::runtime_error("Unsupported PPM token '" + magic + "' in: " + path);
        }
        return img;
    }

    inline std::string ext(const std::string& p) {
        auto d = p.rfind('.');
        if (d == std::string::npos) return "";
        std::string e = p.substr(d+1);
        for (auto& c : e) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return e;
    }

    /**
     * Convert JPEG/PNG/TIFF → PPM via macOS sips (ships with every macOS).
     * Fully offline – no internet required.
     */
    inline RawImage convert_via_sips(const std::string& src) {
        std::string tmp = "/tmp/spine_sips_"
            + std::to_string(reinterpret_cast<std::uintptr_t>(&src)) + ".ppm";
        std::string cmd = "sips -s format ppm \"" + src
            + "\" --out \"" + tmp + "\" > /dev/null 2>&1";
        if (std::system(cmd.c_str()) != 0) {
            std::remove(tmp.c_str());
            throw std::runtime_error(
                "Image conversion failed for: " + src +
                "\n  Ensure sips is available (macOS) and the file is a valid JPEG/PNG/TIFF.");
        }
        RawImage img = load_ppm(tmp);
        std::remove(tmp.c_str());
        return img;
    }

    inline RawImage load_image(const std::string& path) {
        std::string e = ext(path);
        if (e == "ppm" || e == "pgm" || e == "pbm") return load_ppm(path);
        if (e == "jpg" || e == "jpeg" || e == "png" ||
            e == "tiff" || e == "tif"  || e == "bmp") return convert_via_sips(path);
        // Unknown extension: try PPM parser directly
        return load_ppm(path);
    }

} // namespace ImageIO

/* bilinear resize to target_h × target_w */
inline RawImage resize_bilinear(const RawImage& src, int target_h, int target_w) {
    RawImage dst;
    dst.width  = target_w;
    dst.height = target_h;
    dst.pixels.resize(target_h * target_w);

    double scale_h = static_cast<double>(src.height) / target_h;
    double scale_w = static_cast<double>(src.width)  / target_w;

    for (int dy = 0; dy < target_h; ++dy) {
        for (int dx = 0; dx < target_w; ++dx) {
            double sy = (dy + 0.5) * scale_h - 0.5;
            double sx = (dx + 0.5) * scale_w - 0.5;

            int y0 = std::max(0, static_cast<int>(std::floor(sy)));
            int x0 = std::max(0, static_cast<int>(std::floor(sx)));
            int y1 = std::min(src.height - 1, y0 + 1);
            int x1 = std::min(src.width  - 1, x0 + 1);

            double fy = sy - y0, fx = sx - x0;

            double v = (1-fy)*(1-fx)*src.pixels[y0*src.width+x0]
                     + (1-fy)*  fx  *src.pixels[y0*src.width+x1]
                     +   fy *(1-fx)*src.pixels[y1*src.width+x0]
                     +   fy *  fx  *src.pixels[y1*src.width+x1];
            dst.pixels[dy*target_w+dx] = v;
        }
    }
    return dst;
}

/* ─────────────────────────────────────────────────────────────────────────── */
/*  SpineCNN                                                                    */
/* ─────────────────────────────────────────────────────────────────────────── */
class SpineCNN {
public:
    /* ── Architecture constants ── */
    static constexpr int INPUT_H   = 128;
    static constexpr int INPUT_W   = 128;
    static constexpr int INPUT_C   = 1;

    // After Conv1(no-pad,k=3): 126×126; MaxPool(2): 63×63
    // After Conv2(no-pad,k=3): 61×61;  MaxPool(2): 30×30
    // After Conv3(no-pad,k=3): 28×28;  MaxPool(2): 14×14
    static constexpr int FLAT_SIZE = 32 * 14 * 14;   // 6272
    static constexpr int FC1_OUT   = 128;
    static constexpr int NUM_CLASS = 2;

private:
    // Layers
    ConvolutionLayer conv1;   //  1→8,  k=3
    ConvolutionLayer conv2;   //  8→16, k=3
    ConvolutionLayer conv3;   // 16→32, k=3

    ActivationFunctions::ReLU relu1, relu2, relu3;  // conv block activations
    ActivationFunctions::ReLU relu_fc;              // FC1 activation (SEPARATE instance)
    MaxPoolingLayer pool1, pool2, pool3;

    FullyConnectedLayer fc1;   // 6272→128
    FullyConnectedLayer fc2;   // 128→2
    ActivationFunctions::Softmax softmax;

    // Cached for heatmap
    Tensor last_conv_act;
    bool weights_loaded = false;

    /* ── helpers ── */
    static Tensor flatten(const Tensor& t) {
        Tensor out({t.get_total_size()}, false);
        out.get_data() = t.get_data();
        return out;
    }
    static Tensor unflatten(const Tensor& t, int c, int h, int w) {
        Tensor out({c,h,w}, false);
        out.get_data() = t.get_data();
        return out;
    }

    /* SGD update helpers */
    static void sgd_conv(ConvolutionLayer& l, double lr) {
        auto& w = l.get_weights().get_data();
        auto& b = l.get_bias().get_data();
        const auto& wg = l.get_weight_grad().get_data();
        const auto& bg = l.get_bias_grad().get_data();
        for (std::size_t i = 0; i < w.size(); ++i) w[i] -= lr * wg[i];
        for (std::size_t i = 0; i < b.size(); ++i) b[i] -= lr * bg[i];
    }
    static void sgd_fc(FullyConnectedLayer& l, double lr) {
        auto& w = l.get_weights().get_data();
        auto& b = l.get_bias().get_data();
        const auto& wg = l.get_weight_grad().get_data();
        const auto& bg = l.get_bias_grad().get_data();
        for (std::size_t i = 0; i < w.size(); ++i) w[i] -= lr * wg[i];
        for (std::size_t i = 0; i < b.size(); ++i) b[i] -= lr * bg[i];
    }

    /* Binary weight I/O */
    static void write_layer_weights(std::ostream& os, ConvolutionLayer& l) {
        const auto& w = l.get_weights().get_data();
        const auto& b = l.get_bias().get_data();
        std::size_t ws = w.size(), bs = b.size();
        os.write(reinterpret_cast<const char*>(&ws), sizeof(ws));
        os.write(reinterpret_cast<const char*>(w.data()), ws*sizeof(double));
        os.write(reinterpret_cast<const char*>(&bs), sizeof(bs));
        os.write(reinterpret_cast<const char*>(b.data()), bs*sizeof(double));
    }
    static void read_layer_weights(std::istream& is, ConvolutionLayer& l) {
        std::size_t ws; is.read(reinterpret_cast<char*>(&ws), sizeof(ws));
        auto& w = l.get_weights().get_data(); w.resize(ws);
        is.read(reinterpret_cast<char*>(w.data()), ws*sizeof(double));
        std::size_t bs; is.read(reinterpret_cast<char*>(&bs), sizeof(bs));
        auto& b = l.get_bias().get_data(); b.resize(bs);
        is.read(reinterpret_cast<char*>(b.data()), bs*sizeof(double));
    }
    static void write_fc_weights(std::ostream& os, FullyConnectedLayer& l) {
        const auto& w = l.get_weights().get_data();
        const auto& b = l.get_bias().get_data();
        std::size_t ws = w.size(), bs = b.size();
        os.write(reinterpret_cast<const char*>(&ws), sizeof(ws));
        os.write(reinterpret_cast<const char*>(w.data()), ws*sizeof(double));
        os.write(reinterpret_cast<const char*>(&bs), sizeof(bs));
        os.write(reinterpret_cast<const char*>(b.data()), bs*sizeof(double));
    }
    static void read_fc_weights(std::istream& is, FullyConnectedLayer& l) {
        std::size_t ws; is.read(reinterpret_cast<char*>(&ws), sizeof(ws));
        auto& w = l.get_weights().get_data(); w.resize(ws);
        is.read(reinterpret_cast<char*>(w.data()), ws*sizeof(double));
        std::size_t bs; is.read(reinterpret_cast<char*>(&bs), sizeof(bs));
        auto& b = l.get_bias().get_data(); b.resize(bs);
        is.read(reinterpret_cast<char*>(b.data()), bs*sizeof(double));
    }

public:
    SpineCNN()
        : conv1(1,  8, 3, 1, 0, true),
          conv2(8,  16, 3, 1, 0, true),
          conv3(16, 32, 3, 1, 0, true),
          pool1(2), pool2(2), pool3(2),
          fc1(FLAT_SIZE, FC1_OUT, true),
          fc2(FC1_OUT, NUM_CLASS, true)
    {}

    void load_weights(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return;   // silently use random weights
        read_layer_weights(f, conv1);
        read_layer_weights(f, conv2);
        read_layer_weights(f, conv3);
        read_fc_weights   (f, fc1);
        read_fc_weights   (f, fc2);
        weights_loaded = true;
    }

    void save_weights(const std::string& path) const {
        std::ofstream f(path, std::ios::binary);
        if (!f.is_open()) throw std::runtime_error("Cannot write: " + path);
        write_layer_weights(f, const_cast<ConvolutionLayer&>(conv1));
        write_layer_weights(f, const_cast<ConvolutionLayer&>(conv2));
        write_layer_weights(f, const_cast<ConvolutionLayer&>(conv3));
        write_fc_weights   (f, const_cast<FullyConnectedLayer&>(fc1));
        write_fc_weights   (f, const_cast<FullyConnectedLayer&>(fc2));
    }

    bool are_weights_loaded() const { return weights_loaded; }

    /* ── Stats ── */
    int get_param_count() const {
        // Conv1: 8*1*3*3 + 8  = 80
        // Conv2: 16*8*3*3 + 16 = 1168
        // Conv3: 32*16*3*3 + 32 = 4640
        // FC1: 6272*128 + 128 = 802 944
        // FC2: 128*2 + 2 = 258
        return 80 + 1168 + 4640 + 802944 + 258;
    }

    double get_model_size_mb() const {
        return get_param_count() * 8.0 / (1024.0 * 1024.0); // double = 8 bytes
    }

    /* ── Image → Tensor preprocessing ── */
    Tensor preprocess(const std::string& image_path) {
        RawImage img = ImageIO::load_image(image_path);
        img = resize_bilinear(img, INPUT_H, INPUT_W);
        const double mean = 0.5, std = 0.5;
        Tensor t({1, INPUT_H, INPUT_W}, false);
        for (int i = 0; i < INPUT_H; ++i)
            for (int j = 0; j < INPUT_W; ++j)
                t(0, i, j) = (img.pixels[i*INPUT_W+j] - mean) / std;
        return t;
    }

    /* ── Forward pass (inference) ── */
    Tensor forward(const Tensor& input) {
        // Block 1
        Tensor r1 = relu1.forward(conv1.forward(input));
        Tensor p1 = pool1.forward(r1);                      // [8,63,63]
        // Block 2
        Tensor r2 = relu2.forward(conv2.forward(p1));
        Tensor p2 = pool2.forward(r2);                      // [16,30,30]
        // Block 3
        Tensor r3 = relu3.forward(conv3.forward(p2));
        last_conv_act = r3;                                 // heatmap cache
        Tensor p3 = pool3.forward(r3);                      // [32,14,14]
        // FC
        Tensor flat   = flatten(p3);                        // [6272]
        Tensor h      = relu_fc.forward(fc1.forward(flat)); // [128] uses relu_fc
        Tensor probs  = softmax.forward(fc2.forward(h));    // [2]
        return probs;
    }

    /* ── Training: one SGD step, returns cross-entropy loss ── */
    double train_step(const Tensor& input, int label, double lr) {
        /* Forward */
        Tensor r1   = relu1.forward(conv1.forward(input));
        Tensor p1   = pool1.forward(r1);
        Tensor r2   = relu2.forward(conv2.forward(p1));
        Tensor p2   = pool2.forward(r2);
        Tensor r3   = relu3.forward(conv3.forward(p2));
        last_conv_act = r3;
        Tensor p3   = pool3.forward(r3);
        Tensor flat   = flatten(p3);
        Tensor fc1out = fc1.forward(flat);
        Tensor h      = relu_fc.forward(fc1out);
        Tensor logits = fc2.forward(h);
        Tensor probs  = softmax.forward(logits);

        /* Cross-entropy loss */
        const auto& pv = probs.get_data();
        double loss = -std::log(std::max(pv[label], 1e-10));

        /* Combined Softmax + CrossEntropy gradient: probs - one_hot(label) */
        Tensor g({NUM_CLASS}, false);
        auto& gd = g.get_data();
        for (int i = 0; i < NUM_CLASS; ++i)
            gd[i] = pv[i] - (i == label ? 1.0 : 0.0);

        /* Backward */
        g   = fc2.backward(g);                                         // [128]
        g   = relu_fc.backward(g);                                     // [128]
        g   = fc1.backward(g);                                         // [6272]
        Tensor g3d = unflatten(g, 32, 14, 14);                        // [32,14,14]
        g3d = conv3.backward(relu3.backward(pool3.backward(g3d)));    // [16,30,30]
        g3d = conv2.backward(relu2.backward(pool2.backward(g3d)));    // [8,63,63]
        conv1.backward(relu1.backward(pool1.backward(g3d)));          // input grad (discarded)

        /* SGD weight update */
        sgd_conv(conv1, lr); sgd_conv(conv2, lr); sgd_conv(conv3, lr);
        sgd_fc  (fc1,   lr); sgd_fc  (fc2,   lr);

        return loss;
    }

    /* ── Heatmap from last conv activation ── */
    // Returns a 2-D activation map (channel-averaged) resized to heatmap_side×heatmap_side
    std::vector<std::vector<double>> get_heatmap(int heatmap_side = 32) const {
        if (last_conv_act.get_total_size() == 0)
            return std::vector<std::vector<double>>(heatmap_side,
                                                    std::vector<double>(heatmap_side, 0.0));

        const auto& shape = last_conv_act.get_shape(); // [32, 28, 28]
        int C = shape[0], H = shape[1], W = shape[2];

        // Channel-wise mean
        std::vector<double> cam(H * W, 0.0);
        const auto& d = last_conv_act.get_data();
        for (int c = 0; c < C; ++c)
            for (int h = 0; h < H; ++h)
                for (int w = 0; w < W; ++w)
                    cam[h*W+w] += d[c*(H*W)+h*W+w];
        for (auto& v : cam) v /= C;

        // Normalise to [0,1]
        double mn = *std::min_element(cam.begin(), cam.end());
        double mx = *std::max_element(cam.begin(), cam.end());
        double rng = (mx - mn) < 1e-8 ? 1.0 : (mx - mn);
        for (auto& v : cam) v = (v - mn) / rng;

        // Nearest-neighbor resize to heatmap_side×heatmap_side
        std::vector<std::vector<double>> heat(heatmap_side, std::vector<double>(heatmap_side));
        for (int dy = 0; dy < heatmap_side; ++dy) {
            int sy = static_cast<int>(dy * H / heatmap_side);
            for (int dx = 0; dx < heatmap_side; ++dx) {
                int sx = static_cast<int>(dx * W / heatmap_side);
                heat[dy][dx] = cam[sy*W+sx];
            }
        }
        return heat;
    }
};

#endif // SPINE_CNN_HPP
