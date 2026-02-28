#ifndef POOLING_LAYERS_HPP
#define POOLING_LAYERS_HPP

#include "../utils/Tensor.hpp"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <limits>

/**
 * Max Pooling Layer
 * 
 * Mathematical Formulation:
 * Forward: Y[c,i,j] = max_{0≤u<p, 0≤v<p} X[c, i*s + u, j*s + v]
 * Backward: ∂L/∂X[c,x,y] = Σ_{i,j} Δ[c,i,j] * 1{(x,y) is argmax in window (i,j)}
 * 
 * Implementation stores argmax indices for exact gradient routing
 */
class MaxPoolingLayer {
private:
    int pool_size;
    int stride;
    
    // Store argmax indices for backward pass
    // argmax_indices[output_idx] = {input_c, input_h, input_w}
    std::vector<std::vector<int>> argmax_indices;
    std::vector<int> output_shape;
    std::vector<int> input_shape;
    
public:
    MaxPoolingLayer(int pool_size, int stride = -1) 
        : pool_size(pool_size), stride(stride == -1 ? pool_size : stride) {
        if (pool_size <= 0 || this->stride <= 0) {
            throw std::invalid_argument("Pool size and stride must be positive");
        }
    }
    
    /**
     * Forward pass
     * Input shape: [C, H, W]
     * Output shape: [C, H_out, W_out] where H_out = (H - pool_size) / stride + 1
     */
    Tensor forward(const Tensor& input) {
        if (input.get_dims() != 3) {
            throw std::invalid_argument("Input must be 3D: [C, H, W]");
        }
        
        input_shape = input.get_shape();
        int c_in = input_shape[0];
        int h_in = input_shape[1];
        int w_in = input_shape[2];
        
        // Compute output dimensions
        int h_out = (h_in - pool_size) / stride + 1;
        int w_out = (w_in - pool_size) / stride + 1;
        
        if (h_out <= 0 || w_out <= 0) {
            throw std::invalid_argument("Invalid output dimensions for pooling");
        }
        
        output_shape = {c_in, h_out, w_out};
        Tensor output(output_shape, false);
        
        // Reset argmax storage
        int total_output_elements = c_in * h_out * w_out;
        argmax_indices.clear();
        argmax_indices.resize(total_output_elements);
        
        // Max pooling operation
        for (int c = 0; c < c_in; ++c) {
            for (int oh = 0; oh < h_out; ++oh) {
                for (int ow = 0; ow < w_out; ++ow) {
                    double max_val = -std::numeric_limits<double>::infinity();
                    int max_ih = -1, max_iw = -1;
                    
                    // Find maximum in pooling window
                    for (int u = 0; u < pool_size; ++u) {
                        for (int v = 0; v < pool_size; ++v) {
                            int ih = oh * stride + u;
                            int iw = ow * stride + v;
                            
                            if (ih < h_in && iw < w_in) {
                                double val = input(c, ih, iw);
                                if (val > max_val) {
                                    max_val = val;
                                    max_ih = ih;
                                    max_iw = iw;
                                }
                            }
                        }
                    }
                    
                    output(c, oh, ow) = max_val;
                    
                    // Store argmax indices
                    int output_idx = c * h_out * w_out + oh * w_out + ow;
                    argmax_indices[output_idx] = {c, max_ih, max_iw};
                }
            }
        }
        
        return output;
    }
    
    /**
     * Backward pass
     * grad_output shape: [C, H_out, W_out]
     * Returns grad_input shape: [C, H_in, W_in]
     */
    Tensor backward(const Tensor& grad_output) {
        if (argmax_indices.empty()) {
            throw std::runtime_error("Must call forward before backward");
        }
        
        const auto& grad_shape = grad_output.get_shape();
        if (grad_shape != output_shape) {
            throw std::invalid_argument("Gradient output shape doesn't match expected output shape");
        }
        
        Tensor grad_input(input_shape, false);
        grad_input.fill(0.0);
        
        int h_out = output_shape[1];
        int w_out = output_shape[2];
        
        // Route gradients to argmax positions
        for (int c = 0; c < output_shape[0]; ++c) {
            for (int oh = 0; oh < h_out; ++oh) {
                for (int ow = 0; ow < w_out; ++ow) {
                    double delta = grad_output(c, oh, ow);
                    
                    int output_idx = c * h_out * w_out + oh * w_out + ow;
                    const auto& argmax = argmax_indices[output_idx];
                    
                    int max_c = argmax[0];
                    int max_h = argmax[1];
                    int max_w = argmax[2];
                    
                    if (max_h >= 0 && max_w >= 0) {
                        grad_input(max_c, max_h, max_w) += delta;
                    }
                }
            }
        }
        
        return grad_input;
    }
    
    // Getters
    int get_pool_size() const { return pool_size; }
    int get_stride() const { return stride; }
    const std::vector<int>& get_output_shape() const { return output_shape; }
    
    void print_info() const {
        std::cout << "MaxPoolingLayer:" << std::endl;
        std::cout << "  Pool size: " << pool_size << "x" << pool_size << std::endl;
        std::cout << "  Stride: " << stride << std::endl;
    }
};

/**
 * Average Pooling Layer
 * 
 * Mathematical Formulation:
 * Forward: Y[c,i,j] = (1/p²) * Σ_{u=0}^{p-1} Σ_{v=0}^{p-1} X[c, i*s + u, j*s + v]
 * Backward: ∂L/∂X[c,x,y] = Σ_{i,j} Δ[c,i,j] * (1/p²) for windows that include (x,y)
 */
class AveragePoolingLayer {
private:
    int pool_size;
    int stride;
    
    std::vector<int> output_shape;
    std::vector<int> input_shape;
    
public:
    AveragePoolingLayer(int pool_size, int stride = -1)
        : pool_size(pool_size), stride(stride == -1 ? pool_size : stride) {
        if (pool_size <= 0 || this->stride <= 0) {
            throw std::invalid_argument("Pool size and stride must be positive");
        }
    }
    
    /**
     * Forward pass
     * Input shape: [C, H, W]
     * Output shape: [C, H_out, W_out]
     */
    Tensor forward(const Tensor& input) {
        if (input.get_dims() != 3) {
            throw std::invalid_argument("Input must be 3D: [C, H, W]");
        }
        
        input_shape = input.get_shape();
        int c_in = input_shape[0];
        int h_in = input_shape[1];
        int w_in = input_shape[2];
        
        // Compute output dimensions
        int h_out = (h_in - pool_size) / stride + 1;
        int w_out = (w_in - pool_size) / stride + 1;
        
        if (h_out <= 0 || w_out <= 0) {
            throw std::invalid_argument("Invalid output dimensions for pooling");
        }
        
        output_shape = {c_in, h_out, w_out};
        Tensor output(output_shape, false);
        
        double pool_area = static_cast<double>(pool_size * pool_size);
        
        // Average pooling operation
        for (int c = 0; c < c_in; ++c) {
            for (int oh = 0; oh < h_out; ++oh) {
                for (int ow = 0; ow < w_out; ++ow) {
                    double sum = 0.0;
                    int count = 0;
                    
                    // Sum over pooling window
                    for (int u = 0; u < pool_size; ++u) {
                        for (int v = 0; v < pool_size; ++v) {
                            int ih = oh * stride + u;
                            int iw = ow * stride + v;
                            
                            if (ih < h_in && iw < w_in) {
                                sum += input(c, ih, iw);
                                count++;
                            }
                        }
                    }
                    
                    // Compute average
                    output(c, oh, ow) = (count > 0) ? sum / count : 0.0;
                }
            }
        }
        
        return output;
    }
    
    /**
     * Backward pass
     * grad_output shape: [C, H_out, W_out]
     * Returns grad_input shape: [C, H_in, W_in]
     */
    Tensor backward(const Tensor& grad_output) {
        if (input_shape.empty()) {
            throw std::runtime_error("Must call forward before backward");
        }
        
        const auto& grad_shape = grad_output.get_shape();
        if (grad_shape != output_shape) {
            throw std::invalid_argument("Gradient output shape doesn't match expected output shape");
        }
        
        Tensor grad_input(input_shape, false);
        grad_input.fill(0.0);
        
        int h_out = output_shape[1];
        int w_out = output_shape[2];
        int h_in = input_shape[1];
        int w_in = input_shape[2];
        
        // Distribute gradients uniformly over pooling windows
        for (int c = 0; c < output_shape[0]; ++c) {
            for (int oh = 0; oh < h_out; ++oh) {
                for (int ow = 0; ow < w_out; ++ow) {
                    double delta = grad_output(c, oh, ow);
                    
                    // Count valid positions in this window
                    int count = 0;
                    for (int u = 0; u < pool_size; ++u) {
                        for (int v = 0; v < pool_size; ++v) {
                            int ih = oh * stride + u;
                            int iw = ow * stride + v;
                            if (ih < h_in && iw < w_in) {
                                count++;
                            }
                        }
                    }
                    
                    if (count > 0) {
                        double grad_per_element = delta / count;
                        
                        // Distribute gradient
                        for (int u = 0; u < pool_size; ++u) {
                            for (int v = 0; v < pool_size; ++v) {
                                int ih = oh * stride + u;
                                int iw = ow * stride + v;
                                
                                if (ih < h_in && iw < w_in) {
                                    grad_input(c, ih, iw) += grad_per_element;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        return grad_input;
    }
    
    // Getters
    int get_pool_size() const { return pool_size; }
    int get_stride() const { return stride; }
    const std::vector<int>& get_output_shape() const { return output_shape; }
    
    void print_info() const {
        std::cout << "AveragePoolingLayer:" << std::endl;
        std::cout << "  Pool size: " << pool_size << "x" << pool_size << std::endl;
        std::cout << "  Stride: " << stride << std::endl;
    }
};

/**
 * Adaptive Average Pooling
 * Pools to a specific output size regardless of input size
 */
class AdaptiveAveragePoolingLayer {
private:
    int output_h, output_w;
    std::vector<int> input_shape;
    
public:
    AdaptiveAveragePoolingLayer(int output_size) 
        : output_h(output_size), output_w(output_size) {}
    
    AdaptiveAveragePoolingLayer(int output_h, int output_w)
        : output_h(output_h), output_w(output_w) {}
    
    Tensor forward(const Tensor& input) {
        if (input.get_dims() != 3) {
            throw std::invalid_argument("Input must be 3D: [C, H, W]");
        }
        
        input_shape = input.get_shape();
        int c_in = input_shape[0];
        int h_in = input_shape[1];
        int w_in = input_shape[2];
        
        Tensor output({c_in, output_h, output_w}, false);
        
        for (int c = 0; c < c_in; ++c) {
            for (int oh = 0; oh < output_h; ++oh) {
                for (int ow = 0; ow < output_w; ++ow) {
                    // Compute adaptive pooling region
                    int h_start = (oh * h_in) / output_h;
                    int h_end = ((oh + 1) * h_in + output_h - 1) / output_h;
                    int w_start = (ow * w_in) / output_w;
                    int w_end = ((ow + 1) * w_in + output_w - 1) / output_w;
                    
                    double sum = 0.0;
                    int count = 0;
                    
                    for (int ih = h_start; ih < h_end && ih < h_in; ++ih) {
                        for (int iw = w_start; iw < w_end && iw < w_in; ++iw) {
                            sum += input(c, ih, iw);
                            count++;
                        }
                    }
                    
                    output(c, oh, ow) = (count > 0) ? sum / count : 0.0;
                }
            }
        }
        
        return output;
    }
    
    Tensor backward(const Tensor& grad_output) {
        if (input_shape.empty()) {
            throw std::runtime_error("Must call forward before backward");
        }
        
        Tensor grad_input(input_shape, false);
        grad_input.fill(0.0);
        
        int c_in = input_shape[0];
        int h_in = input_shape[1];
        int w_in = input_shape[2];
        
        for (int c = 0; c < c_in; ++c) {
            for (int oh = 0; oh < output_h; ++oh) {
                for (int ow = 0; ow < output_w; ++ow) {
                    double delta = grad_output(c, oh, ow);
                    
                    // Compute adaptive pooling region
                    int h_start = (oh * h_in) / output_h;
                    int h_end = ((oh + 1) * h_in + output_h - 1) / output_h;
                    int w_start = (ow * w_in) / output_w;
                    int w_end = ((ow + 1) * w_in + output_w - 1) / output_w;
                    
                    int count = 0;
                    for (int ih = h_start; ih < h_end && ih < h_in; ++ih) {
                        for (int iw = w_start; iw < w_end && iw < w_in; ++iw) {
                            count++;
                        }
                    }
                    
                    if (count > 0) {
                        double grad_per_element = delta / count;
                        
                        for (int ih = h_start; ih < h_end && ih < h_in; ++ih) {
                            for (int iw = w_start; iw < w_end && iw < w_in; ++iw) {
                                grad_input(c, ih, iw) += grad_per_element;
                            }
                        }
                    }
                }
            }
        }
        
        return grad_input;
    }
    
    void print_info() const {
        std::cout << "AdaptiveAveragePoolingLayer:" << std::endl;
        std::cout << "  Output size: " << output_h << "x" << output_w << std::endl;
    }
};

/**
 * Global Average Pooling
 * Pools each channel to a single value (adaptive pooling to 1x1)
 */
class GlobalAveragePoolingLayer {
private:
    std::vector<int> input_shape;
    
public:
    Tensor forward(const Tensor& input) {
        if (input.get_dims() != 3) {
            throw std::invalid_argument("Input must be 3D: [C, H, W]");
        }
        
        input_shape = input.get_shape();
        int c_in = input_shape[0];
        int h_in = input_shape[1];
        int w_in = input_shape[2];
        
        Tensor output({c_in}, false);
        
        for (int c = 0; c < c_in; ++c) {
            double sum = 0.0;
            for (int h = 0; h < h_in; ++h) {
                for (int w = 0; w < w_in; ++w) {
                    sum += input(c, h, w);
                }
            }
            output[c] = sum / (h_in * w_in);
        }
        
        return output;
    }
    
    Tensor backward(const Tensor& grad_output) {
        if (input_shape.empty()) {
            throw std::runtime_error("Must call forward before backward");
        }
        
        if (grad_output.get_shape()[0] != input_shape[0]) {
            throw std::invalid_argument("Gradient output channels don't match input");
        }
        
        Tensor grad_input(input_shape, false);
        
        int c_in = input_shape[0];
        int h_in = input_shape[1];
        int w_in = input_shape[2];
        double scale = 1.0 / (h_in * w_in);
        
        for (int c = 0; c < c_in; ++c) {
            double delta = grad_output[c];
            for (int h = 0; h < h_in; ++h) {
                for (int w = 0; w < w_in; ++w) {
                    grad_input(c, h, w) = delta * scale;
                }
            }
        }
        
        return grad_input;
    }
    
    void print_info() const {
        std::cout << "GlobalAveragePoolingLayer: pools to [C] shape" << std::endl;
    }
};

#endif // POOLING_LAYERS_HPP