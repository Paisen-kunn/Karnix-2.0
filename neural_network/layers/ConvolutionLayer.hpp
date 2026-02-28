#ifndef CONVOLUTION_LAYER_HPP
#define CONVOLUTION_LAYER_HPP

#include "../utils/Tensor.hpp"
#include <vector>
#include <stdexcept>
#include <algorithm>

/**
 * Convolution Layer Implementation
 * 
 * Mathematical Formulation (channels-first):
 * Input: X ∈ R^(C_in × H_in × W_in)
 * Kernel: K ∈ R^(C_out × C_in × k_h × k_w)
 * Bias: b ∈ R^(C_out)
 * Output: Y ∈ R^(C_out × H_out × W_out)
 * 
 * Forward:
 * Y[o,i,j] = Σ_c Σ_u Σ_v K[o,c,u,v] * X[c, i*s_h + u - p_h, j*s_w + v - p_w] + b[o]
 * 
 * Backward:
 * ∂L/∂b[o] = Σ_{i,j} Δ[o,i,j]
 * ∂L/∂K[o,c,u,v] = Σ_{i,j} Δ[o,i,j] * X[c, i*s_h + u - p_h, j*s_w + v - p_w]
 * ∂L/∂X[c,x,y] = Σ_o Σ_u Σ_v Δ[o,i,j] * K[o,c,u,v] where x = i*s_h + u - p_h, y = j*s_w + v - p_w
 */
class ConvolutionLayer {
private:
    // Layer parameters
    int in_channels, out_channels;
    int kernel_height, kernel_width;
    int stride_h, stride_w;
    int pad_h, pad_w;
    bool use_bias;
    
    // Weights and biases
    Tensor weights;  // Shape: [C_out, C_in, k_h, k_w]
    Tensor bias;     // Shape: [C_out]
    
    // Gradients
    Tensor weight_grad;
    Tensor bias_grad;
    
    // Cache for backward pass
    Tensor input_cache;
    std::vector<int> output_shape;
    
    // Helper function to check if indices are valid (with padding)
    bool is_valid_input_index(int c, int h, int w, const std::vector<int>& input_shape) const {
        if (c < 0 || c >= input_shape[0]) return false;
        if (h < 0 || h >= input_shape[1]) return false;
        if (w < 0 || w >= input_shape[2]) return false;
        return true;
    }
    
    // Get padded input value (returns 0 if out of bounds)
    double get_padded_input(const Tensor& input, int c, int h, int w) const {
        const auto& input_shape = input.get_shape();
        
        // Apply padding offset
        int actual_h = h - pad_h;
        int actual_w = w - pad_w;
        
        if (actual_h < 0 || actual_h >= input_shape[1] || 
            actual_w < 0 || actual_w >= input_shape[2]) {
            return 0.0; // Padding value
        }
        
        return input(c, actual_h, actual_w);
    }
    
public:
    ConvolutionLayer(int in_channels, int out_channels, 
                    int kernel_size, int stride = 1, int padding = 0, bool use_bias = true)
        : in_channels(in_channels), out_channels(out_channels),
          kernel_height(kernel_size), kernel_width(kernel_size),
          stride_h(stride), stride_w(stride),
          pad_h(padding), pad_w(padding),
          use_bias(use_bias) {
        
        initialize_weights();
    }
    
    ConvolutionLayer(int in_channels, int out_channels,
                    int kernel_height, int kernel_width,
                    int stride_h, int stride_w,
                    int pad_h, int pad_w, bool use_bias = true)
        : in_channels(in_channels), out_channels(out_channels),
          kernel_height(kernel_height), kernel_width(kernel_width),
          stride_h(stride_h), stride_w(stride_w),
          pad_h(pad_h), pad_w(pad_w),
          use_bias(use_bias) {
        
        initialize_weights();
    }
    
    void initialize_weights() {
        // Initialize weights with Xavier/Glorot initialization
        weights = Tensor({out_channels, in_channels, kernel_height, kernel_width}, true);
        weights.xavier_init();
        
        weight_grad = Tensor({out_channels, in_channels, kernel_height, kernel_width}, false);
        
        if (use_bias) {
            bias = Tensor({out_channels}, true);
            bias.fill(0.0); // Initialize bias to zero
            bias_grad = Tensor({out_channels}, false);
        }
    }
    
    /**
     * Forward pass
     * Input shape: [C_in, H_in, W_in]
     * Output shape: [C_out, H_out, W_out]
     */
    Tensor forward(const Tensor& input) {
        if (input.get_dims() != 3) {
            throw std::invalid_argument("Input must be 3D: [C_in, H_in, W_in]");
        }
        
        const auto& input_shape = input.get_shape();
        if (input_shape[0] != in_channels) {
            throw std::invalid_argument("Input channels don't match layer configuration");
        }
        
        // Cache input for backward pass
        input_cache = input.clone();
        
        // Compute output dimensions
        int h_out = (input_shape[1] + 2 * pad_h - kernel_height) / stride_h + 1;
        int w_out = (input_shape[2] + 2 * pad_w - kernel_width) / stride_w + 1;
        
        if (h_out <= 0 || w_out <= 0) {
            throw std::invalid_argument("Invalid output dimensions. Check padding and stride.");
        }
        
        output_shape = {out_channels, h_out, w_out};
        Tensor output(output_shape, false);
        
        // Convolution operation: Y[o,i,j] = Σ_c Σ_u Σ_v K[o,c,u,v] * X[c, i*s_h + u - p_h, j*s_w + v - p_w] + b[o]
        for (int o = 0; o < out_channels; ++o) {
            for (int i = 0; i < h_out; ++i) {
                for (int j = 0; j < w_out; ++j) {
                    double sum = 0.0;
                    
                    // Convolution sum over input channels and kernel
                    for (int c = 0; c < in_channels; ++c) {
                        for (int u = 0; u < kernel_height; ++u) {
                            for (int v = 0; v < kernel_width; ++v) {
                                int input_h = i * stride_h + u;
                                int input_w = j * stride_w + v;
                                
                                double input_val = get_padded_input(input, c, input_h, input_w);
                                double weight_val = weights(o, c, u, v);
                                
                                sum += weight_val * input_val;
                            }
                        }
                    }
                    
                    // Add bias
                    if (use_bias) {
                        sum += bias[o];
                    }
                    
                    output(o, i, j) = sum;
                }
            }
        }
        
        return output;
    }
    
    /**
     * Backward pass
     * grad_output shape: [C_out, H_out, W_out]
     * Returns grad_input shape: [C_in, H_in, W_in]
     */
    Tensor backward(const Tensor& grad_output) {
        if (input_cache.get_data().empty()) {
            throw std::runtime_error("Must call forward before backward");
        }
        
        const auto& grad_shape = grad_output.get_shape();
        const auto& input_shape = input_cache.get_shape();
        
        if (grad_shape != output_shape) {
            throw std::invalid_argument("Gradient output shape doesn't match expected output shape");
        }
        
        // Initialize gradients
        weight_grad.fill(0.0);
        if (use_bias) {
            bias_grad.fill(0.0);
        }
        
        Tensor grad_input(input_shape, false);
        grad_input.fill(0.0);
        
        int h_out = grad_shape[1];
        int w_out = grad_shape[2];
        
        // Compute gradients
        for (int o = 0; o < out_channels; ++o) {
            for (int i = 0; i < h_out; ++i) {
                for (int j = 0; j < w_out; ++j) {
                    double delta = grad_output(o, i, j);
                    
                    // Gradient w.r.t. bias: ∂L/∂b[o] = Σ_{i,j} Δ[o,i,j]
                    if (use_bias) {
                        bias_grad[o] += delta;
                    }
                    
                    // Gradient w.r.t. weights and input
                    for (int c = 0; c < in_channels; ++c) {
                        for (int u = 0; u < kernel_height; ++u) {
                            for (int v = 0; v < kernel_width; ++v) {
                                int input_h = i * stride_h + u;
                                int input_w = j * stride_w + v;
                                
                                // Gradient w.r.t. weights: ∂L/∂K[o,c,u,v] = Σ_{i,j} Δ[o,i,j] * X[c, input_h, input_w]
                                double input_val = get_padded_input(input_cache, c, input_h, input_w);
                                weight_grad(o, c, u, v) += delta * input_val;
                                
                                // Gradient w.r.t. input: ∂L/∂X[c,x,y] = Σ_o Σ_u Σ_v Δ[o,i,j] * K[o,c,u,v]
                                int actual_h = input_h - pad_h;
                                int actual_w = input_w - pad_w;
                                
                                if (actual_h >= 0 && actual_h < input_shape[1] &&
                                    actual_w >= 0 && actual_w < input_shape[2]) {
                                    double weight_val = weights(o, c, u, v);
                                    grad_input(c, actual_h, actual_w) += delta * weight_val;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        return grad_input;
    }
    
    // Getters for weights and gradients
    const Tensor& get_weights() const { return weights; }
    const Tensor& get_bias() const { return bias; }
    const Tensor& get_weight_grad() const { return weight_grad; }
    const Tensor& get_bias_grad() const { return bias_grad; }
    
    Tensor& get_weights() { return weights; }
    Tensor& get_bias() { return bias; }
    
    // Parameter count
    int get_parameter_count() const {
        int weight_params = out_channels * in_channels * kernel_height * kernel_width;
        int bias_params = use_bias ? out_channels : 0;
        return weight_params + bias_params;
    }
    
    // Layer info
    void print_info() const {
        std::cout << "ConvolutionLayer:" << std::endl;
        std::cout << "  Input channels: " << in_channels << std::endl;
        std::cout << "  Output channels: " << out_channels << std::endl;
        std::cout << "  Kernel size: " << kernel_height << "x" << kernel_width << std::endl;
        std::cout << "  Stride: " << stride_h << "x" << stride_w << std::endl;
        std::cout << "  Padding: " << pad_h << "x" << pad_w << std::endl;
        std::cout << "  Use bias: " << (use_bias ? "Yes" : "No") << std::endl;
        std::cout << "  Parameters: " << get_parameter_count() << std::endl;
    }
};

/**
 * Utility functions for convolution operations
 */
namespace ConvUtils {
    /**
     * Compute output size for convolution
     * Formula: H_out = ⌊(H_in + 2*pad_h - kernel_h) / stride_h⌋ + 1
     */
    std::vector<int> compute_conv_output_size(const std::vector<int>& input_shape,
                                            int kernel_h, int kernel_w,
                                            int stride_h, int stride_w,
                                            int pad_h, int pad_w) {
        if (input_shape.size() != 3) {
            throw std::invalid_argument("Input shape must be 3D: [C, H, W]");
        }
        
        int h_out = (input_shape[1] + 2 * pad_h - kernel_h) / stride_h + 1;
        int w_out = (input_shape[2] + 2 * pad_w - kernel_w) / stride_w + 1;
        
        return {input_shape[0], h_out, w_out}; // Keep same number of channels for size computation
    }
    
    /**
     * Im2Col transformation for efficient convolution via GEMM
     * Converts convolution to matrix multiplication
     */
    Tensor im2col(const Tensor& input, int kernel_h, int kernel_w,
                  int stride_h, int stride_w, int pad_h, int pad_w) {
        const auto& input_shape = input.get_shape();
        if (input_shape.size() != 3) {
            throw std::invalid_argument("Input must be 3D: [C, H, W]");
        }
        
        int c_in = input_shape[0];
        int h_in = input_shape[1];
        int w_in = input_shape[2];
        
        int h_out = (h_in + 2 * pad_h - kernel_h) / stride_h + 1;
        int w_out = (w_in + 2 * pad_w - kernel_w) / stride_w + 1;
        
        // Output matrix: [kernel_h * kernel_w * c_in, h_out * w_out]
        int col_height = kernel_h * kernel_w * c_in;
        int col_width = h_out * w_out;
        
        Tensor col_matrix({col_height, col_width}, false);
        
        for (int c = 0; c < c_in; ++c) {
            for (int kh = 0; kh < kernel_h; ++kh) {
                for (int kw = 0; kw < kernel_w; ++kw) {
                    int row = c * kernel_h * kernel_w + kh * kernel_w + kw;
                    
                    for (int oh = 0; oh < h_out; ++oh) {
                        for (int ow = 0; ow < w_out; ++ow) {
                            int col = oh * w_out + ow;
                            
                            int ih = oh * stride_h + kh - pad_h;
                            int iw = ow * stride_w + kw - pad_w;
                            
                            double val = 0.0;
                            if (ih >= 0 && ih < h_in && iw >= 0 && iw < w_in) {
                                val = input(c, ih, iw);
                            }
                            
                            col_matrix(row, col) = val;
                        }
                    }
                }
            }
        }
        
        return col_matrix;
    }
    
    /**
     * Col2Im transformation (reverse of Im2Col)
     * Used in backward pass to convert gradients back to input format
     */
    Tensor col2im(const Tensor& col_matrix, const std::vector<int>& input_shape,
                  int kernel_h, int kernel_w, int stride_h, int stride_w,
                  int pad_h, int pad_w) {
        int c_in = input_shape[0];
        int h_in = input_shape[1];
        int w_in = input_shape[2];
        
        int h_out = (h_in + 2 * pad_h - kernel_h) / stride_h + 1;
        int w_out = (w_in + 2 * pad_w - kernel_w) / stride_w + 1;
        
        Tensor input(input_shape, false);
        input.fill(0.0);
        
        for (int c = 0; c < c_in; ++c) {
            for (int kh = 0; kh < kernel_h; ++kh) {
                for (int kw = 0; kw < kernel_w; ++kw) {
                    int row = c * kernel_h * kernel_w + kh * kernel_w + kw;
                    
                    for (int oh = 0; oh < h_out; ++oh) {
                        for (int ow = 0; ow < w_out; ++ow) {
                            int col = oh * w_out + ow;
                            
                            int ih = oh * stride_h + kh - pad_h;
                            int iw = ow * stride_w + kw - pad_w;
                            
                            if (ih >= 0 && ih < h_in && iw >= 0 && iw < w_in) {
                                input(c, ih, iw) += col_matrix(row, col);
                            }
                        }
                    }
                }
            }
        }
        
        return input;
    }
}

#endif // CONVOLUTION_LAYER_HPP