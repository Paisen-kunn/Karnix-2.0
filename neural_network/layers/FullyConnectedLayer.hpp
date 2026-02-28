#ifndef FULLY_CONNECTED_LAYER_HPP
#define FULLY_CONNECTED_LAYER_HPP

#include "../utils/Tensor.hpp"
#include <vector>
#include <stdexcept>
#include <iostream>

/**
 * Fully Connected (Dense) Layer
 * 
 * Mathematical Formulation:
 * Input: a ∈ R^n (vector)
 * Weights: W ∈ R^(m×n) (matrix)
 * Bias: b ∈ R^m (vector)
 * 
 * Forward: z = W*a + b, y = f(z)
 * 
 * Backward (given δ = ∂L/∂z ∈ R^m):
 * ∂L/∂W = δ * a^T (outer product: m×n)
 * ∂L/∂b = δ (summed over batch)
 * ∂L/∂a = W^T * δ
 */
class FullyConnectedLayer {
private:
    int input_size;
    int output_size;
    bool use_bias;
    
    // Parameters
    Tensor weights;  // Shape: [output_size, input_size]
    Tensor bias;     // Shape: [output_size]
    
    // Gradients
    Tensor weight_grad;
    Tensor bias_grad;
    
    // Cache for backward pass
    Tensor input_cache;
    
public:
    FullyConnectedLayer(int input_size, int output_size, bool use_bias = true)
        : input_size(input_size), output_size(output_size), use_bias(use_bias) {
        initialize_weights();
    }
    
    void initialize_weights() {
        // Initialize weights with Xavier/Glorot initialization
        weights = Tensor({output_size, input_size}, true);
        weights.xavier_init();
        
        weight_grad = Tensor({output_size, input_size}, false);
        
        if (use_bias) {
            bias = Tensor({output_size}, true);
            bias.fill(0.0); // Initialize bias to zero
            bias_grad = Tensor({output_size}, false);
        }
    }
    
    /**
     * Forward pass
     * Input can be 1D [input_size] or 2D [batch_size, input_size]
     * Returns 1D [output_size] or 2D [batch_size, output_size]
     */
    Tensor forward(const Tensor& input) {
        const auto& input_shape = input.get_shape();
        
        if (input.get_dims() == 1) {
            // Single sample: [input_size]
            if (input_shape[0] != input_size) {
                throw std::invalid_argument("Input size doesn't match layer configuration");
            }
            return forward_single(input);
        } else if (input.get_dims() == 2) {
            // Batch: [batch_size, input_size]
            if (input_shape[1] != input_size) {
                throw std::invalid_argument("Input size doesn't match layer configuration");
            }
            return forward_batch(input);
        } else {
            throw std::invalid_argument("Input must be 1D or 2D");
        }
    }
    
private:
    /**
     * Forward pass for single sample
     * Input: [input_size]
     * Output: [output_size]
     */
    Tensor forward_single(const Tensor& input) {
        // Cache input for backward pass
        input_cache = input.clone();
        
        Tensor output({output_size}, false);
        
        // Matrix-vector multiplication: z = W*a + b
        for (int i = 0; i < output_size; ++i) {
            double sum = 0.0;
            
            // Dot product of weight row with input
            for (int j = 0; j < input_size; ++j) {
                sum += weights(i, j) * input[j];
            }
            
            // Add bias
            if (use_bias) {
                sum += bias[i];
            }
            
            output[i] = sum;
        }
        
        return output;
    }
    
    /**
     * Forward pass for batch
     * Input: [batch_size, input_size]
     * Output: [batch_size, output_size]
     */
    Tensor forward_batch(const Tensor& input) {
        // Cache input for backward pass
        input_cache = input.clone();
        
        const auto& input_shape = input.get_shape();
        int batch_size = input_shape[0];
        
        Tensor output({batch_size, output_size}, false);
        
        // Batch matrix multiplication: Z = X*W^T + b
        for (int b = 0; b < batch_size; ++b) {
            for (int i = 0; i < output_size; ++i) {
                double sum = 0.0;
                
                // Dot product
                for (int j = 0; j < input_size; ++j) {
                    sum += input(b, j) * weights(i, j);
                }
                
                // Add bias
                if (use_bias) {
                    sum += bias[i];
                }
                
                output(b, i) = sum;
            }
        }
        
        return output;
    }

public:
    /**
     * Backward pass
     * grad_output: gradient w.r.t. layer output
     * Returns: gradient w.r.t. layer input
     */
    Tensor backward(const Tensor& grad_output) {
        if (input_cache.get_data().empty()) {
            throw std::runtime_error("Must call forward before backward");
        }
        
        const auto& grad_shape = grad_output.get_shape();
        const auto& input_shape = input_cache.get_shape();
        
        if (grad_output.get_dims() == 1) {
            // Single sample
            if (grad_shape[0] != output_size) {
                throw std::invalid_argument("Gradient output size doesn't match layer output size");
            }
            return backward_single(grad_output);
        } else if (grad_output.get_dims() == 2) {
            // Batch
            if (grad_shape[1] != output_size) {
                throw std::invalid_argument("Gradient output size doesn't match layer output size");
            }
            return backward_batch(grad_output);
        } else {
            throw std::invalid_argument("Gradient output must be 1D or 2D");
        }
    }
    
private:
    /**
     * Backward pass for single sample
     */
    Tensor backward_single(const Tensor& grad_output) {
        // Initialize gradients
        weight_grad.fill(0.0);
        if (use_bias) {
            bias_grad.fill(0.0);
        }
        
        Tensor grad_input({input_size}, false);
        
        // Compute gradients
        for (int i = 0; i < output_size; ++i) {
            double delta = grad_output[i];
            
            // Gradient w.r.t. bias: ∂L/∂b = δ
            if (use_bias) {
                bias_grad[i] = delta;
            }
            
            // Gradient w.r.t. weights: ∂L/∂W[i,j] = δ[i] * a[j]
            for (int j = 0; j < input_size; ++j) {
                weight_grad(i, j) = delta * input_cache[j];
            }
        }
        
        // Gradient w.r.t. input: ∂L/∂a[j] = Σ_i W[i,j] * δ[i]
        for (int j = 0; j < input_size; ++j) {
            double sum = 0.0;
            for (int i = 0; i < output_size; ++i) {
                sum += weights(i, j) * grad_output[i];
            }
            grad_input[j] = sum;
        }
        
        return grad_input;
    }
    
    /**
     * Backward pass for batch
     */
    Tensor backward_batch(const Tensor& grad_output) {
        const auto& grad_shape = grad_output.get_shape();
        const auto& input_shape = input_cache.get_shape();
        int batch_size = grad_shape[0];
        
        // Initialize gradients
        weight_grad.fill(0.0);
        if (use_bias) {
            bias_grad.fill(0.0);
        }
        
        Tensor grad_input(input_shape, false);
        grad_input.fill(0.0);
        
        // Accumulate gradients over batch
        for (int b = 0; b < batch_size; ++b) {
            for (int i = 0; i < output_size; ++i) {
                double delta = grad_output(b, i);
                
                // Gradient w.r.t. bias: accumulate across batch
                if (use_bias) {
                    bias_grad[i] += delta;
                }
                
                // Gradient w.r.t. weights: ∂L/∂W[i,j] = Σ_b δ[b,i] * a[b,j]
                for (int j = 0; j < input_size; ++j) {
                    weight_grad(i, j) += delta * input_cache(b, j);
                }
            }
        }
        
        // Gradient w.r.t. input: ∂L/∂a[b,j] = Σ_i W[i,j] * δ[b,i]
        for (int b = 0; b < batch_size; ++b) {
            for (int j = 0; j < input_size; ++j) {
                double sum = 0.0;
                for (int i = 0; i < output_size; ++i) {
                    sum += weights(i, j) * grad_output(b, i);
                }
                grad_input(b, j) = sum;
            }
        }
        
        return grad_input;
    }

public:
    // Getters for weights and gradients
    const Tensor& get_weights() const { return weights; }
    const Tensor& get_bias() const { return bias; }
    const Tensor& get_weight_grad() const { return weight_grad; }
    const Tensor& get_bias_grad() const { return bias_grad; }
    
    Tensor& get_weights() { return weights; }
    Tensor& get_bias() { return bias; }
    
    // Parameter count
    int get_parameter_count() const {
        int weight_params = output_size * input_size;
        int bias_params = use_bias ? output_size : 0;
        return weight_params + bias_params;
    }
    
    // Layer info
    int get_input_size() const { return input_size; }
    int get_output_size() const { return output_size; }
    bool get_use_bias() const { return use_bias; }
    
    void print_info() const {
        std::cout << "FullyConnectedLayer:" << std::endl;
        std::cout << "  Input size: " << input_size << std::endl;
        std::cout << "  Output size: " << output_size << std::endl;
        std::cout << "  Use bias: " << (use_bias ? "Yes" : "No") << std::endl;
        std::cout << "  Parameters: " << get_parameter_count() << std::endl;
    }
    
    /**
     * Apply L2 regularization to weights
     * Adds λ * ||W||² to loss, which adds λ * W to weight gradients
     */
    void apply_l2_regularization(double lambda) {
        auto& weight_data = weight_grad.get_data();
        const auto& weights_data = weights.get_data();
        
        for (int i = 0; i < weight_data.size(); ++i) {
            weight_data[i] += lambda * weights_data[i];
        }
    }
    
    /**
     * Apply L1 regularization to weights
     * Adds λ * ||W||₁ to loss, which adds λ * sign(W) to weight gradients
     */
    void apply_l1_regularization(double lambda) {
        auto& weight_data = weight_grad.get_data();
        const auto& weights_data = weights.get_data();
        
        for (int i = 0; i < weight_data.size(); ++i) {
            if (weights_data[i] > 0) {
                weight_data[i] += lambda;
            } else if (weights_data[i] < 0) {
                weight_data[i] -= lambda;
            }
        }
    }
};

/**
 * Flatten Layer
 * Reshapes tensor to vector for fully connected layers
 */
class FlattenLayer {
private:
    std::vector<int> input_shape;
    
public:
    /**
     * Forward pass: reshape tensor to vector
     * Input: any shape tensor
     * Output: 1D tensor with same total elements
     */
    Tensor forward(const Tensor& input) {
        input_shape = input.get_shape();
        int total_size = input.get_total_size();
        
        Tensor output({total_size}, false);
        output.get_data() = input.get_data(); // Copy data
        
        return output;
    }
    
    /**
     * Backward pass: reshape gradient vector back to original tensor shape
     */
    Tensor backward(const Tensor& grad_output) {
        if (input_shape.empty()) {
            throw std::runtime_error("Must call forward before backward");
        }
        
        if (grad_output.get_total_size() != 
            std::accumulate(input_shape.begin(), input_shape.end(), 1, std::multiplies<int>())) {
            throw std::invalid_argument("Gradient size doesn't match flattened input size");
        }
        
        Tensor grad_input(input_shape, false);
        grad_input.get_data() = grad_output.get_data(); // Copy data
        
        return grad_input;
    }
    
    const std::vector<int>& get_input_shape() const { return input_shape; }
    
    void print_info() const {
        std::cout << "FlattenLayer: reshapes tensor to vector" << std::endl;
    }
};

/**
 * Dropout Layer
 * Randomly sets elements to zero during training for regularization
 */
class DropoutLayer {
private:
    double dropout_rate;
    bool training;
    Tensor mask; // Binary mask for dropped elements
    
public:
    DropoutLayer(double dropout_rate = 0.5) 
        : dropout_rate(dropout_rate), training(true) {
        if (dropout_rate < 0.0 || dropout_rate >= 1.0) {
            throw std::invalid_argument("Dropout rate must be in [0, 1)");
        }
    }
    
    void set_training(bool is_training) { training = is_training; }
    bool is_training() const { return training; }
    
    /**
     * Forward pass
     * During training: randomly drop elements and scale remaining
     * During inference: pass through unchanged
     */
    Tensor forward(const Tensor& input) {
        if (!training || dropout_rate == 0.0) {
            // Inference mode or no dropout
            return input.clone();
        }
        
        Tensor output = input.clone();
        mask = Tensor(input.get_shape(), false);
        
        double keep_prob = 1.0 - dropout_rate;
        double scale = 1.0 / keep_prob; // Inverted dropout scaling
        
        auto& output_data = output.get_data();
        auto& mask_data = mask.get_data();
        
        for (int i = 0; i < output_data.size(); ++i) {
            double random_val = static_cast<double>(rand()) / RAND_MAX;
            if (random_val < keep_prob) {
                mask_data[i] = 1.0;
                output_data[i] *= scale; // Scale to maintain expected value
            } else {
                mask_data[i] = 0.0;
                output_data[i] = 0.0;
            }
        }
        
        return output;
    }
    
    /**
     * Backward pass
     * Route gradients only through non-dropped elements
     */
    Tensor backward(const Tensor& grad_output) {
        if (!training || dropout_rate == 0.0) {
            // No dropout applied, pass gradients through unchanged
            return grad_output.clone();
        }
        
        if (mask.get_data().empty()) {
            throw std::runtime_error("Must call forward before backward");
        }
        
        Tensor grad_input = grad_output.clone();
        
        const auto& mask_data = mask.get_data();
        auto& grad_data = grad_input.get_data();
        
        double keep_prob = 1.0 - dropout_rate;
        double scale = 1.0 / keep_prob;
        
        for (int i = 0; i < grad_data.size(); ++i) {
            if (mask_data[i] == 0.0) {
                grad_data[i] = 0.0; // Zero gradient for dropped elements
            } else {
                grad_data[i] *= scale; // Apply same scaling as forward pass
            }
        }
        
        return grad_input;
    }
    
    double get_dropout_rate() const { return dropout_rate; }
    
    void print_info() const {
        std::cout << "DropoutLayer:" << std::endl;
        std::cout << "  Dropout rate: " << dropout_rate << std::endl;
        std::cout << "  Training mode: " << (training ? "Yes" : "No") << std::endl;
    }
};

#endif // FULLY_CONNECTED_LAYER_HPP