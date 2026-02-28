#ifndef IMAGE_PROCESSOR_HPP
#define IMAGE_PROCESSOR_HPP

#include "Tensor.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>

/**
 * Image Processing Utilities for CNN
 * 
 * Handles image loading, conversion to tensors, and preprocessing
 * for neural network input. Supports RGB and grayscale images.
 * 
 * Mathematical conversions:
 * - Pixel normalization: X[c,i,j] = I[c,i,j] / 255.0
 * - Channel ordering: Channels-first format [C, H, W]
 * - Data augmentation: rotation, scaling, noise injection
 */
class ImageProcessor {
public:
    // Simple RGB pixel structure
    struct RGBPixel {
        unsigned char r, g, b;
        RGBPixel(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0) 
            : r(red), g(green), b(blue) {}
    };
    
    // Simple image structure
    struct Image {
        std::vector<std::vector<RGBPixel>> pixels;
        int width, height;
        
        Image(int w = 0, int h = 0) : width(w), height(h) {
            pixels.resize(height, std::vector<RGBPixel>(width));
        }
        
        RGBPixel& pixel(int x, int y) {
            return pixels[y][x];
        }
        
        const RGBPixel& pixel(int x, int y) const {
            return pixels[y][x];
        }
    };
    
private:
    bool verbose;
    
public:
    ImageProcessor(bool verbose_mode = true) : verbose(verbose_mode) {}
    
    /**
     * Create synthetic test images for CNN validation
     * 
     * @param width: Image width
     * @param height: Image height
     * @param pattern: Pattern type ("gradient", "checkerboard", "circles", "noise")
     * @return: Generated test image
     */
    Image create_test_image(int width, int height, const std::string& pattern = "gradient") {
        Image img(width, height);
        
        if (pattern == "gradient") {
            // Horizontal gradient from black to white
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    unsigned char value = static_cast<unsigned char>(255.0 * x / width);
                    img.pixel(x, y) = RGBPixel(value, value, value);
                }
            }
        }
        else if (pattern == "checkerboard") {
            // Checkerboard pattern
            int block_size = std::max(1, width / 8);
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    bool is_white = ((x / block_size) + (y / block_size)) % 2 == 0;
                    unsigned char value = is_white ? 255 : 0;
                    img.pixel(x, y) = RGBPixel(value, value, value);
                }
            }
        }
        else if (pattern == "circles") {
            // Concentric circles
            int center_x = width / 2;
            int center_y = height / 2;
            double max_radius = std::min(width, height) / 2.0;
            
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    double distance = std::sqrt((x - center_x) * (x - center_x) + 
                                              (y - center_y) * (y - center_y));
                    double normalized_dist = distance / max_radius;
                    unsigned char value = static_cast<unsigned char>(
                        255.0 * (0.5 + 0.5 * std::sin(normalized_dist * 10.0))
                    );
                    img.pixel(x, y) = RGBPixel(value, value, value);
                }
            }
        }
        else if (pattern == "noise") {
            // Random noise pattern
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    unsigned char r = rand() % 256;
                    unsigned char g = rand() % 256;
                    unsigned char b = rand() % 256;
                    img.pixel(x, y) = RGBPixel(r, g, b);
                }
            }
        }
        else if (pattern == "edges") {
            // Edge detection test pattern
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    unsigned char value = 128; // Base gray
                    
                    // Vertical edges
                    if (x == width/4 || x == 3*width/4) {
                        value = 255;
                    }
                    // Horizontal edges
                    if (y == height/4 || y == 3*height/4) {
                        value = 255;
                    }
                    
                    img.pixel(x, y) = RGBPixel(value, value, value);
                }
            }
        }
        
        if (verbose) {
            std::cout << "Created " << pattern << " test image: " << width << "x" << height << std::endl;
        }
        
        return img;
    }
    
    /**
     * Convert image to tensor with channels-first format [C, H, W]
     * 
     * Mathematical transformation:
     * X[c,i,j] = I[c,i,j] / 255.0
     * 
     * @param img: Input image
     * @param normalize: Whether to normalize pixels to [0,1] range
     * @param grayscale: Whether to convert to grayscale (1 channel)
     * @return: Tensor of shape [C, H, W] where C=1 (grayscale) or C=3 (RGB)
     */
    Tensor image_to_tensor(const Image& img, bool normalize = true, bool grayscale = false) {
        int channels = grayscale ? 1 : 3;
        Tensor tensor({channels, img.height, img.width}, false);
        
        auto& data = tensor.get_data();
        
        for (int y = 0; y < img.height; ++y) {
            for (int x = 0; x < img.width; ++x) {
                const RGBPixel& pixel = img.pixel(x, y);
                
                if (grayscale) {
                    // Convert to grayscale using luminance formula: 0.299*R + 0.587*G + 0.114*B
                    double gray_value = 0.299 * pixel.r + 0.587 * pixel.g + 0.114 * pixel.b;
                    if (normalize) gray_value /= 255.0;
                    
                    int idx = y * img.width + x; // [0, H, W] format
                    data[idx] = gray_value;
                } else {
                    // RGB channels-first format
                    double r_val = normalize ? pixel.r / 255.0 : pixel.r;
                    double g_val = normalize ? pixel.g / 255.0 : pixel.g;
                    double b_val = normalize ? pixel.b / 255.0 : pixel.b;
                    
                    // Channel 0 (Red): [0, H, W]
                    int r_idx = 0 * (img.height * img.width) + y * img.width + x;
                    // Channel 1 (Green): [1, H, W]
                    int g_idx = 1 * (img.height * img.width) + y * img.width + x;
                    // Channel 2 (Blue): [2, H, W]
                    int b_idx = 2 * (img.height * img.width) + y * img.width + x;
                    
                    data[r_idx] = r_val;
                    data[g_idx] = g_val;
                    data[b_idx] = b_val;
                }
            }
        }
        
        if (verbose) {
            std::cout << "Converted image to tensor: [" << channels << ", " << img.height 
                     << ", " << img.width << "]" << std::endl;
            std::cout << "Pixel range: [" << *std::min_element(data.begin(), data.end()) 
                     << ", " << *std::max_element(data.begin(), data.end()) << "]" << std::endl;
        }
        
        return tensor;
    }
    
    /**
     * Convert tensor back to image for visualization
     * 
     * @param tensor: Input tensor of shape [C, H, W]
     * @param denormalize: Whether to scale [0,1] back to [0,255]
     * @return: Reconstructed image
     */
    Image tensor_to_image(const Tensor& tensor, bool denormalize = true) {
        const auto& shape = tensor.get_shape();
        const auto& data = tensor.get_data();
        
        if (shape.size() != 3) {
            throw std::invalid_argument("Tensor must have shape [C, H, W]");
        }
        
        int channels = shape[0];
        int height = shape[1];
        int width = shape[2];
        
        Image img(width, height);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (channels == 1) {
                    // Grayscale
                    int idx = y * width + x;
                    double val = data[idx];
                    if (denormalize) val *= 255.0;
                    val = std::max(0.0, std::min(255.0, val));
                    unsigned char gray = static_cast<unsigned char>(val);
                    img.pixel(x, y) = RGBPixel(gray, gray, gray);
                } else if (channels >= 3) {
                    // RGB
                    int r_idx = 0 * (height * width) + y * width + x;
                    int g_idx = 1 * (height * width) + y * width + x;
                    int b_idx = 2 * (height * width) + y * width + x;
                    
                    double r_val = data[r_idx];
                    double g_val = data[g_idx];
                    double b_val = data[b_idx];
                    
                    if (denormalize) {
                        r_val *= 255.0;
                        g_val *= 255.0;
                        b_val *= 255.0;
                    }
                    
                    r_val = std::max(0.0, std::min(255.0, r_val));
                    g_val = std::max(0.0, std::min(255.0, g_val));
                    b_val = std::max(0.0, std::min(255.0, b_val));
                    
                    img.pixel(x, y) = RGBPixel(
                        static_cast<unsigned char>(r_val),
                        static_cast<unsigned char>(g_val),
                        static_cast<unsigned char>(b_val)
                    );
                }
            }
        }
        
        return img;
    }
    
    /**
     * Visualize tensor as ASCII art
     * 
     * @param tensor: Input tensor
     * @param channel: Which channel to visualize (for multi-channel tensors)
     * @param width: ASCII art width
     */
    void visualize_tensor_ascii(const Tensor& tensor, int channel = 0, int width = 60) {
        const auto& shape = tensor.get_shape();
        const auto& data = tensor.get_data();
        
        if (shape.size() < 2) {
            std::cout << "Cannot visualize tensor with less than 2 dimensions" << std::endl;
            return;
        }
        
        int height, tensor_width;
        int start_idx = 0;
        
        if (shape.size() == 2) {
            height = shape[0];
            tensor_width = shape[1];
        } else if (shape.size() == 3) {
            if (channel >= shape[0]) {
                std::cout << "Channel " << channel << " out of range (0-" << shape[0]-1 << ")" << std::endl;
                return;
            }
            height = shape[1];
            tensor_width = shape[2];
            start_idx = channel * height * tensor_width;
        } else {
            std::cout << "Cannot visualize tensor with more than 3 dimensions" << std::endl;
            return;
        }
        
        // Find min/max for normalization
        double min_val = *std::min_element(data.begin() + start_idx, 
                                          data.begin() + start_idx + height * tensor_width);
        double max_val = *std::max_element(data.begin() + start_idx, 
                                          data.begin() + start_idx + height * tensor_width);
        
        std::cout << "\nTensor Visualization (Channel " << channel << "):" << std::endl;
        std::cout << "Range: [" << min_val << ", " << max_val << "]" << std::endl;
        std::cout << "Shape: [" << height << ", " << tensor_width << "]" << std::endl;
        std::cout << std::string(width + 2, '=') << std::endl;
        
        const std::string intensity_chars = " .:-=+*#%@";
        
        for (int y = 0; y < height; ++y) {
            std::cout << "|";
            for (int x = 0; x < tensor_width; ++x) {
                int idx = start_idx + y * tensor_width + x;
                double normalized = (max_val > min_val) ? 
                    (data[idx] - min_val) / (max_val - min_val) : 0.0;
                
                int char_idx = static_cast<int>(normalized * (intensity_chars.length() - 1));
                char_idx = std::max(0, std::min(static_cast<int>(intensity_chars.length() - 1), char_idx));
                
                std::cout << intensity_chars[char_idx];
            }
            std::cout << "|" << std::endl;
        }
        std::cout << std::string(width + 2, '=') << std::endl;
    }
    
    /**
     * Data augmentation functions
     */
    
    /**
     * Add Gaussian noise to tensor
     * 
     * @param tensor: Input tensor
     * @param noise_level: Standard deviation of noise (0.0 to 1.0)
     * @return: Noisy tensor
     */
    Tensor add_noise(const Tensor& tensor, double noise_level = 0.1) {
        Tensor noisy_tensor = tensor.clone();
        auto& data = noisy_tensor.get_data();
        
        for (auto& val : data) {
            // Simple pseudo-random noise
            double noise = noise_level * (2.0 * static_cast<double>(rand()) / RAND_MAX - 1.0);
            val += noise;
            val = std::max(0.0, std::min(1.0, val)); // Clamp to [0,1]
        }
        
        return noisy_tensor;
    }
    
    /**
     * Create feature visualization for CNN filter
     * 
     * @param filter_weights: Weights tensor of shape [out_ch, in_ch, kh, kw]
     * @param filter_idx: Which filter to visualize
     * @return: Visualization tensor
     */
    Tensor visualize_filter(const Tensor& filter_weights, int filter_idx = 0) {
        const auto& shape = filter_weights.get_shape();
        if (shape.size() != 4) {
            throw std::invalid_argument("Filter weights must have shape [out_ch, in_ch, kh, kw]");
        }
        
        int out_channels = shape[0];
        int in_channels = shape[1];
        int kernel_h = shape[2];
        int kernel_w = shape[3];
        
        if (filter_idx >= out_channels) {
            throw std::invalid_argument("Filter index out of range");
        }
        
        // Create visualization tensor for one filter
        Tensor vis_tensor({in_channels, kernel_h, kernel_w}, false);
        const auto& weights_data = filter_weights.get_data();
        auto& vis_data = vis_tensor.get_data();
        
        // Extract weights for specific filter
        int filter_start = filter_idx * in_channels * kernel_h * kernel_w;
        std::copy(weights_data.begin() + filter_start,
                 weights_data.begin() + filter_start + in_channels * kernel_h * kernel_w,
                 vis_data.begin());
        
        // Normalize for visualization
        double min_val = *std::min_element(vis_data.begin(), vis_data.end());
        double max_val = *std::max_element(vis_data.begin(), vis_data.end());
        
        if (max_val > min_val) {
            for (auto& val : vis_data) {
                val = (val - min_val) / (max_val - min_val);
            }
        }
        
        return vis_tensor;
    }
    
    /**
     * Create activation heatmap from feature map
     * 
     * Mathematical formula:
     * heatmap(i,j) = Σ_c |Y[c,i,j]|
     * 
     * @param feature_map: Feature map tensor of shape [C, H, W]
     * @return: Heatmap tensor of shape [1, H, W]
     */
    Tensor create_activation_heatmap(const Tensor& feature_map) {
        const auto& shape = feature_map.get_shape();
        if (shape.size() != 3) {
            throw std::invalid_argument("Feature map must have shape [C, H, W]");
        }
        
        int channels = shape[0];
        int height = shape[1];
        int width = shape[2];
        
        Tensor heatmap({1, height, width}, false);
        const auto& feature_data = feature_map.get_data();
        auto& heatmap_data = heatmap.get_data();
        
        // Sum absolute values across channels
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double sum = 0.0;
                for (int c = 0; c < channels; ++c) {
                    int idx = c * height * width + y * width + x;
                    sum += std::abs(feature_data[idx]);
                }
                
                int heatmap_idx = y * width + x;
                heatmap_data[heatmap_idx] = sum;
            }
        }
        
        // Normalize heatmap
        double max_val = *std::max_element(heatmap_data.begin(), heatmap_data.end());
        if (max_val > 0.0) {
            for (auto& val : heatmap_data) {
                val /= max_val;
            }
        }
        
        return heatmap;
    }
    
    /**
     * Print detailed tensor statistics
     */
    void print_tensor_stats(const Tensor& tensor, const std::string& name = "Tensor") {
        const auto& data = tensor.get_data();
        const auto& shape = tensor.get_shape();
        
        double sum = 0.0;
        double min_val = data[0];
        double max_val = data[0];
        
        for (double val : data) {
            sum += val;
            min_val = std::min(min_val, val);
            max_val = std::max(max_val, val);
        }
        
        double mean = sum / data.size();
        
        double variance = 0.0;
        for (double val : data) {
            variance += (val - mean) * (val - mean);
        }
        variance /= data.size();
        double std_dev = std::sqrt(variance);
        
        std::cout << "\n=== " << name << " Statistics ===" << std::endl;
        std::cout << "Shape: [";
        for (int i = 0; i < shape.size(); ++i) {
            std::cout << shape[i];
            if (i < shape.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        std::cout << "Elements: " << data.size() << std::endl;
        std::cout << "Range: [" << min_val << ", " << max_val << "]" << std::endl;
        std::cout << "Mean: " << mean << std::endl;
        std::cout << "Std Dev: " << std_dev << std::endl;
        
        // Show sparsity (percentage of near-zero values)
        int near_zero_count = 0;
        double threshold = 0.01;
        for (double val : data) {
            if (std::abs(val) < threshold) {
                near_zero_count++;
            }
        }
        double sparsity = 100.0 * near_zero_count / data.size();
        std::cout << "Sparsity (|val| < " << threshold << "): " << sparsity << "%" << std::endl;
    }
};

#endif // IMAGE_PROCESSOR_HPP