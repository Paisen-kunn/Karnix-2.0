#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_ttf.h>

#include "medical/SpineCNN.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <dirent.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const int WINDOW_WIDTH = 1440;
const int WINDOW_HEIGHT = 900;
const char* MODEL_PATH = "./models/osteoarthritis_cnn.weights";
const char* DEFAULT_IMAGES = "./data/osteoarthritis/screen";
std::string task_name = "OSTEOARTHRITIS";
std::string negative_name = "NO OA";
std::string positive_name = "OA";

struct Color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a = 255;
};

struct Stage {
    std::string short_name;
    std::string title;
    std::string description;
    std::string formula;
    const Tensor* tensor;
};

void set_color(SDL_Renderer* renderer, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void fill_rect(SDL_Renderer* renderer, const SDL_Rect& rect, Color color) {
    set_color(renderer, color);
    SDL_RenderFillRect(renderer, &rect);
}

void stroke_rect(SDL_Renderer* renderer, const SDL_Rect& rect, Color color, int width = 1) {
    set_color(renderer, color);
    for (int offset = 0; offset < width; ++offset) {
        SDL_Rect border{rect.x + offset, rect.y + offset,
                        rect.w - offset * 2, rect.h - offset * 2};
        SDL_RenderDrawRect(renderer, &border);
    }
}

void draw_text(SDL_Renderer* renderer, TTF_Font* font, const std::string& text,
               int x, int y, Color color) {
    SDL_Color sdl_color{color.r, color.g, color.b, color.a};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), sdl_color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destination{x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    if (texture) {
        SDL_RenderCopy(renderer, texture, nullptr, &destination);
        SDL_DestroyTexture(texture);
    }
}

void draw_centered_text(SDL_Renderer* renderer, TTF_Font* font,
                        const std::string& text, const SDL_Rect& bounds, Color color) {
    int width = 0;
    int height = 0;
    TTF_SizeUTF8(font, text.c_str(), &width, &height);
    draw_text(renderer, font, text,
              bounds.x + (bounds.w - width) / 2,
              bounds.y + (bounds.h - height) / 2, color);
}

std::string basename(const std::string& path) {
    std::size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::vector<std::string> scan_images(const std::string& directory) {
    std::vector<std::string> normal;
    std::vector<std::string> positive;
    DIR* handle = opendir(directory.c_str());
    if (!handle) return {};
    dirent* entry;
    while ((entry = readdir(handle)) != nullptr) {
        std::string name = entry->d_name;
        if (name.size() < 4 || name.substr(name.size() - 4) != ".pgm") continue;
        std::string path = directory + "/" + name;
        if (name.find("no_oa_") == 0 || name.find("no_stenosis_") == 0 ||
            name.find("normal_") == 0) normal.push_back(path);
        else positive.push_back(path);
    }
    closedir(handle);
    std::sort(normal.begin(), normal.end());
    std::sort(positive.begin(), positive.end());
    std::vector<std::string> result;
    for (std::size_t index = 0; result.size() < 10 &&
         (index < normal.size() || index < positive.size()); ++index) {
        if (index < normal.size()) result.push_back(normal[index]);
        if (result.size() < 10 && index < positive.size()) result.push_back(positive[index]);
    }
    return result;
}

SDL_Texture* texture_from_plane(SDL_Renderer* renderer, const Tensor& tensor,
                                int channel, bool input_image = false) {
    const std::vector<int>& shape = tensor.get_shape();
    int height = shape.size() == 3 ? shape[1] : 1;
    int width = shape.size() == 3 ? shape[2] : shape[0];
    const std::vector<double>& data = tensor.get_data();
    int plane_offset = shape.size() == 3 ? channel * height * width : 0;
    double minimum = data[plane_offset];
    double maximum = data[plane_offset];
    for (int index = 0; index < height * width; ++index) {
        minimum = std::min(minimum, data[plane_offset + index]);
        maximum = std::max(maximum, data[plane_offset + index]);
    }
    double range = std::max(1e-9, maximum - minimum);
    std::vector<Uint32> pixels(width * height);
    for (int index = 0; index < width * height; ++index) {
        double value = data[plane_offset + index];
        double normalized = input_image ? (value * 0.5 + 0.5) : (value - minimum) / range;
        Uint8 gray = static_cast<Uint8>(std::max(0.0, std::min(255.0, normalized * 255.0)));
        pixels[index] = 0xff000000u | (static_cast<Uint32>(gray) << 16) |
                        (static_cast<Uint32>(gray) << 8) | gray;
    }
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STATIC, width, height);
    if (texture) {
        SDL_UpdateTexture(texture, nullptr, pixels.data(), width * sizeof(Uint32));
        SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
    }
    return texture;
}

void draw_tensor_plane(SDL_Renderer* renderer, const Tensor& tensor, int channel,
                       const SDL_Rect& destination, bool input_image = false) {
    SDL_Texture* texture = texture_from_plane(renderer, tensor, channel, input_image);
    if (!texture) return;
    SDL_RenderCopy(renderer, texture, nullptr, &destination);
    SDL_DestroyTexture(texture);
    stroke_rect(renderer, destination, Color{182, 193, 205}, 1);
}

void draw_arrow(SDL_Renderer* renderer, int x1, int y, int x2, Color color) {
    set_color(renderer, color);
    SDL_RenderDrawLine(renderer, x1, y, x2, y);
    SDL_RenderDrawLine(renderer, x2, y, x2 - 7, y - 5);
    SDL_RenderDrawLine(renderer, x2, y, x2 - 7, y + 5);
}

std::string dimensions(const Tensor& tensor) {
    const std::vector<int>& shape = tensor.get_shape();
    std::ostringstream output;
    for (std::size_t index = 0; index < shape.size(); ++index) {
        if (index) output << " x ";
        output << shape[index];
    }
    return output.str();
}

void draw_feature_grid(SDL_Renderer* renderer, TTF_Font* small_font,
                       const Tensor& tensor, const SDL_Rect& bounds) {
    const std::vector<int>& shape = tensor.get_shape();
    if (shape.size() != 3) return;
    int channels = std::min(shape[0], 8);
    int columns = 4;
    int rows = (channels + columns - 1) / columns;
    int gap = 12;
    int cell_width = (bounds.w - gap * (columns - 1)) / columns;
    int cell_height = (bounds.h - gap * (rows - 1)) / rows;
    int side = std::min(cell_width, cell_height - 20);
    for (int channel = 0; channel < channels; ++channel) {
        int column = channel % columns;
        int row = channel / columns;
        SDL_Rect image{bounds.x + column * (cell_width + gap),
                       bounds.y + row * (cell_height + gap), side, side};
        draw_tensor_plane(renderer, tensor, channel, image);
        draw_text(renderer, small_font, "CHANNEL " + std::to_string(channel + 1),
                  image.x, image.y + image.h + 2, Color{75, 85, 99});
    }
}

void draw_dense(SDL_Renderer* renderer, TTF_Font* small_font,
                const Tensor& tensor, const SDL_Rect& bounds) {
    const std::vector<double>& values = tensor.get_data();
    double maximum = *std::max_element(values.begin(), values.end());
    maximum = std::max(maximum, 1e-9);
    int count = static_cast<int>(values.size());
    int gap = 5;
    int bar_width = std::max(4, (bounds.w - gap * (count - 1)) / count);
    for (int index = 0; index < count; ++index) {
        int height = static_cast<int>((values[index] / maximum) * (bounds.h - 25));
        SDL_Rect bar{bounds.x + index * (bar_width + gap),
                     bounds.y + bounds.h - height - 20, bar_width, height};
        fill_rect(renderer, bar, values[index] > 0
            ? Color{15, 118, 110} : Color{203, 213, 225});
    }
    draw_text(renderer, small_font, "32 learned activations feed the final 2-class layer",
              bounds.x, bounds.y + bounds.h - 18, Color{75, 85, 99});
}

void draw_probability(SDL_Renderer* renderer, TTF_Font* body_font,
                      TTF_Font* heading_font, const SDL_Rect& bounds,
                      double normal, double positive_probability) {
    draw_text(renderer, heading_font,
              positive_probability > normal ? positive_name : negative_name,
              bounds.x, bounds.y,
              positive_probability > normal ? Color{185, 28, 28} : Color{4, 120, 87});
    const std::vector<std::pair<std::string, double>> values = {
        {negative_name, normal}, {positive_name, positive_probability}
    };
    for (int index = 0; index < 2; ++index) {
        int y = bounds.y + 62 + index * 78;
        std::ostringstream percentage;
        percentage << std::fixed << std::setprecision(1) << values[index].second * 100.0 << "%";
        draw_text(renderer, body_font, values[index].first, bounds.x, y, Color{31, 41, 55});
        draw_text(renderer, body_font, percentage.str(), bounds.x + bounds.w - 65, y,
                  Color{31, 41, 55});
        SDL_Rect track{bounds.x, y + 31, bounds.w, 18};
        fill_rect(renderer, track, Color{226, 232, 240});
        SDL_Rect value{track.x, track.y,
                       static_cast<int>(track.w * values[index].second), track.h};
        fill_rect(renderer, value, index == 0 ? Color{14, 116, 144} : Color{220, 38, 38});
    }
}

std::vector<Stage> make_stages(const SpineCNN::ForwardTrace& trace) {
    return {
        {"INPUT", "Input radiograph", "The X-ray is resized and normalized.",
         "1 channel x 64 x 64 pixels", &trace.input},
        {"CONV 1", "Convolution block 1", "Four learned 3x3 kernels scan for local edges.",
         "Y[o,i,j] = sum K[o,u,v] X[i+u,j+v] + b", &trace.conv1},
        {"RELU 1", "ReLU activation", "Negative responses become zero; useful features remain.",
         "ReLU(x) = max(0, x)", &trace.relu1},
        {"POOL 1", "2x2 max pooling", "Each 2x2 region keeps its strongest response.",
         "62x62 becomes 31x31 per channel", &trace.pool1},
        {"CONV 2", "Convolution block 2", "Eight kernels combine earlier edges into textures.",
         "4 input maps -> 8 learned feature maps", &trace.relu2},
        {"POOL 2", "Second pooling", "Spatial detail shrinks while strong evidence survives.",
         "29x29 becomes 14x14 per channel", &trace.pool2},
        {"CONV 3", "Convolution block 3", "Sixteen maps encode higher-level joint patterns.",
         "8 input maps -> 16 pathology feature maps", &trace.relu3},
        {"POOL 3", "Final pooling", "The final maps become a compact 16x6x6 representation.",
         "16 x 6 x 6 = 576 values", &trace.pool3},
        {"DENSE", "Dense decision layer", "All 576 values connect to 32 learned activations.",
         "576 -> 32 -> 2 logits", &trace.dense},
        {"SOFTMAX", "Class probabilities", "Softmax converts the two logits into probabilities.",
         "P(class i) = exp(zi) / sum exp(zj)", &trace.probabilities}
    };
}

TTF_Font* open_font(int size, bool bold) {
    const char* paths[] = {
        bold ? "/System/Library/Fonts/SFNS.ttf" : "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/SFNSMono.ttf",
        "/Library/Fonts/Arial.ttf"
    };
    for (const char* path : paths) {
        TTF_Font* font = TTF_OpenFont(path, size);
        if (font) {
            if (bold) TTF_SetFontStyle(font, TTF_STYLE_BOLD);
            return font;
        }
    }
    return nullptr;
}

void render_scene(SDL_Renderer* renderer, TTF_Font* title_font, TTF_Font* heading_font,
                  TTF_Font* body_font, TTF_Font* small_font,
                  const std::string& image_path, int image_index, int image_count,
                  const SpineCNN::ForwardTrace& trace, int stage_index,
                  double stage_progress, bool paused, double speed) {
    fill_rect(renderer, SDL_Rect{0, 0, WINDOW_WIDTH, WINDOW_HEIGHT}, Color{244, 247, 249});
    fill_rect(renderer, SDL_Rect{0, 0, WINDOW_WIDTH, 82}, Color{255, 255, 255});
    fill_rect(renderer, SDL_Rect{0, 80, WINDOW_WIDTH, 2}, Color{14, 116, 144});
    draw_text(renderer, title_font, "KARNIX " + task_name + " CNN", 34, 18, Color{17, 24, 39});
    draw_text(renderer, body_font, "Native C++ forward-pass visualizer", 36, 52, Color{75, 85, 99});
    std::ostringstream counter;
    counter << "IMAGE " << image_index + 1 << " / " << image_count << "   " << basename(image_path);
    draw_text(renderer, body_font, counter.str(), 1010, 31, Color{31, 41, 55});

    std::vector<Stage> stages = make_stages(trace);
    int pipeline_x = 34;
    int pipeline_y = 104;
    int node_width = 118;
    int node_height = 52;
    int node_gap = 20;
    for (int index = 0; index < static_cast<int>(stages.size()); ++index) {
        SDL_Rect node{pipeline_x + index * (node_width + node_gap), pipeline_y,
                      node_width, node_height};
        Color fill = index < stage_index ? Color{220, 252, 231}
                    : index == stage_index ? Color{207, 250, 254}
                    : Color{255, 255, 255};
        Color border = index < stage_index ? Color{22, 163, 74}
                      : index == stage_index ? Color{8, 145, 178}
                      : Color{203, 213, 225};
        fill_rect(renderer, node, fill);
        stroke_rect(renderer, node, border, index == stage_index ? 3 : 1);
        draw_centered_text(renderer, small_font, stages[index].short_name, node, Color{31, 41, 55});
        if (index + 1 < static_cast<int>(stages.size())) {
            draw_arrow(renderer, node.x + node.w + 3, node.y + node.h / 2,
                       node.x + node.w + node_gap - 4, Color{100, 116, 139});
        }
    }

    SDL_Rect left_panel{34, 182, 400, 614};
    SDL_Rect center_panel{454, 182, 600, 614};
    SDL_Rect right_panel{1074, 182, 332, 614};
    for (const SDL_Rect& panel : {left_panel, center_panel, right_panel}) {
        fill_rect(renderer, panel, Color{255, 255, 255});
        stroke_rect(renderer, panel, Color{218, 226, 234});
    }

    draw_text(renderer, heading_font, "REAL KNEE X-RAY", 58, 208, Color{17, 24, 39});
    SDL_Rect xray{70, 262, 328, 328};
    draw_tensor_plane(renderer, trace.input, 0, xray, true);
    if (stage_index == 1) {
        int scan_x = static_cast<int>(stage_progress * 56.0);
        int scan_y = (scan_x * 7) % 57;
        SDL_Rect kernel{xray.x + scan_x * xray.w / 64,
                        xray.y + scan_y * xray.h / 64,
                        std::max(12, 3 * xray.w / 64),
                        std::max(12, 3 * xray.h / 64)};
        stroke_rect(renderer, kernel, Color{234, 88, 12}, 3);
        draw_text(renderer, small_font, "3 x 3 LEARNED KERNEL", 70, 603, Color{194, 65, 12});
    }
    draw_text(renderer, body_font, "Ground truth from filename:", 58, 650, Color{75, 85, 99});
    std::string filename = basename(image_path);
    bool known_positive = filename.find("no_oa_") != 0 &&
                          filename.find("no_stenosis_") != 0 &&
                          filename.find("normal_") != 0;
    draw_text(renderer, heading_font, known_positive ? positive_name : negative_name, 58, 681,
              known_positive ? Color{185, 28, 28} : Color{4, 120, 87});
    draw_text(renderer, small_font, "Ground-truth label in prepared dataset", 58, 730,
              Color{100, 116, 139});

    const Stage& stage = stages[stage_index];
    draw_text(renderer, heading_font, stage.title, 480, 208, Color{17, 24, 39});
    draw_text(renderer, body_font, stage.description, 480, 244, Color{75, 85, 99});
    SDL_Rect formula_box{480, 278, 548, 48};
    fill_rect(renderer, formula_box, Color{240, 249, 255});
    draw_centered_text(renderer, small_font, stage.formula, formula_box, Color{3, 105, 161});
    draw_text(renderer, small_font, "TENSOR SHAPE: " + dimensions(*stage.tensor),
              480, 342, Color{100, 116, 139});

    SDL_Rect tensor_area{480, 378, 548, 370};
    if (stage_index == 8) {
        draw_dense(renderer, small_font, *stage.tensor, tensor_area);
    } else if (stage_index == 9) {
        draw_probability(renderer, body_font, heading_font, tensor_area,
                         trace.probabilities.get_data()[0], trace.probabilities.get_data()[1]);
    } else {
        draw_feature_grid(renderer, small_font, *stage.tensor, tensor_area);
    }

    draw_text(renderer, heading_font, "WHAT IS HAPPENING", 1098, 208, Color{17, 24, 39});
    const std::vector<std::string> explanations = {
        "Pixels are normalized", "to a stable numeric range.", "", "The same trained weights",
        "are used for every image.", "", "Green stages are complete.", "Cyan is executing now.",
        "", "Feature maps are actual", "intermediate C++ tensors."
    };
    int text_y = 262;
    for (const std::string& line : explanations) {
        draw_text(renderer, body_font, line, 1098, text_y, Color{55, 65, 81});
        text_y += 29;
    }
    SDL_Rect verdict{1098, 610, 284, 128};
    fill_rect(renderer, verdict, Color{248, 250, 252});
    draw_text(renderer, small_font, "CURRENT MODEL OUTPUT", 1116, 628, Color{100, 116, 139});
    double positive_probability = trace.probabilities.get_data()[1];
    std::ostringstream score;
    score << std::fixed << std::setprecision(1) << positive_probability * 100.0
          << "% " << positive_name;
    draw_text(renderer, title_font, score.str(), 1116, 657,
              positive_probability > 0.5 ? Color{185, 28, 28} : Color{4, 120, 87});
    draw_text(renderer, small_font,
              "Prediction: " + (positive_probability > 0.5 ? positive_name : negative_name),
              1116, 706, Color{31, 41, 55});

    fill_rect(renderer, SDL_Rect{0, 822, WINDOW_WIDTH, 78}, Color{255, 255, 255});
    std::ostringstream controls;
    controls << (paused ? "PAUSED" : "PLAYING")
             << "   SPACE Play/Pause   LEFT/RIGHT Step   UP/DOWN Speed   ESC Close"
             << "   Speed " << std::fixed << std::setprecision(1) << speed << "x";
    draw_text(renderer, body_font, controls.str(), 34, 844, Color{55, 65, 81});
    SDL_Rect progress_track{34, 880, 1372, 6};
    fill_rect(renderer, progress_track, Color{226, 232, 240});
    double overall = (image_index * stages.size() + stage_index + stage_progress) /
                     (image_count * static_cast<double>(stages.size()));
    fill_rect(renderer, SDL_Rect{progress_track.x, progress_track.y,
              static_cast<int>(progress_track.w * overall), progress_track.h}, Color{8, 145, 178});
}

} 

int main(int argc, char** argv) {
    SDL_SetMainReady();
    std::string image_directory = argc >= 2 ? argv[1] : DEFAULT_IMAGES;
    std::string model_path = argc >= 3 ? argv[2] : MODEL_PATH;
    if (argc >= 4) task_name = argv[3];
    if (argc >= 5) negative_name = argv[4];
    if (argc >= 6) positive_name = argv[5];
    std::vector<std::string> images = scan_images(image_directory);
    if (images.empty()) {
        std::cerr << "No supported images found in " << image_directory << "\n";
        return 1;
    }

    SpineCNN model;
    model.load_weights(model_path);
    if (!model.are_weights_loaded()) {
        std::cerr << "Trained model missing. Run ./setup.sh first.\n";
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 || TTF_Init() != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << "\n";
        return 1;
    }

    std::string window_title = "Karnix " + task_name + " CNN - Native C++ Visualizer";
    SDL_Window* window = SDL_CreateWindow(
        window_title.c_str(), SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : nullptr;
    if (!window || !renderer) {
        std::cerr << "Cannot create native window: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    TTF_Font* title_font = open_font(28, true);
    TTF_Font* heading_font = open_font(21, true);
    TTF_Font* body_font = open_font(16, false);
    TTF_Font* small_font = open_font(13, true);
    if (!title_font || !heading_font || !body_font || !small_font) {
        std::cerr << "Unable to load a system font: " << TTF_GetError() << "\n";
        return 1;
    }

    int image_index = 0;
    int stage_index = 0;
    bool running = true;
    bool paused = false;
    double speed = 1.0;
    Uint64 stage_started = SDL_GetTicks64();
    const double stage_duration = 1450.0;
    Tensor input = model.preprocess(images[image_index]);
    SpineCNN::ForwardTrace trace = model.forward_with_trace(input);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                else if (event.key.keysym.sym == SDLK_SPACE) paused = !paused;
                else if (event.key.keysym.sym == SDLK_UP) speed = std::min(4.0, speed + 0.25);
                else if (event.key.keysym.sym == SDLK_DOWN) speed = std::max(0.25, speed - 0.25);
                else if (event.key.keysym.sym == SDLK_RIGHT) {
                    stage_index++;
                    if (stage_index >= 10) {
                        stage_index = 0;
                        image_index = (image_index + 1) % static_cast<int>(images.size());
                        trace = model.forward_with_trace(model.preprocess(images[image_index]));
                    }
                    stage_started = SDL_GetTicks64();
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                    stage_index--;
                    if (stage_index < 0) {
                        stage_index = 9;
                        image_index = (image_index - 1 + images.size()) % images.size();
                        trace = model.forward_with_trace(model.preprocess(images[image_index]));
                    }
                    stage_started = SDL_GetTicks64();
                }
            }
        }

        Uint64 now = SDL_GetTicks64();
        double elapsed = (now - stage_started) * speed;
        if (!paused && elapsed >= stage_duration) {
            stage_index++;
            stage_started = now;
            if (stage_index >= 10) {
                stage_index = 0;
                image_index = (image_index + 1) % static_cast<int>(images.size());
                trace = model.forward_with_trace(model.preprocess(images[image_index]));
            }
            elapsed = 0.0;
        }
        double progress = paused ? 0.5 : std::min(1.0, elapsed / stage_duration);
        render_scene(renderer, title_font, heading_font, body_font, small_font,
                     images[image_index], image_index, images.size(), trace,
                     stage_index, progress, paused, speed);
        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(title_font);
    TTF_CloseFont(heading_font);
    TTF_CloseFont(body_font);
    TTF_CloseFont(small_font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
