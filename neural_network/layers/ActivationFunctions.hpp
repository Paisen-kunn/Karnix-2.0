#ifndef ACTIVATION_FUNCTIONS_HPP
#define ACTIVATION_FUNCTIONS_HPP

#include "../utils/Tensor.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

/**
 * Activation Functions with exact mathematical formulations
 * All functions implement both forward and backward passes
 * Derivatives are computed analytically for efficiency
 */
namespace ActivationFunctions {

    /**
     * ReLU (Rectified Linear Unit)
     * Forward: f(z) = max(0, z)
     * Backward: f'(z) = 1 if z > 0, else 0
     */
    class ReLU {
    private:
        Tensor mask; // Store mask for backward pass
        
    public:
        Tensor forward(const Tensor& input) {
            Tensor output(input.get_shape(), false);
            mask = Tensor(input.get_shape(), false);
            
            const auto& input_data = input.get_data();
            auto& output_data = output.get_data();
            auto& mask_data = mask.get_data();
            
            for (int i = 0; i < input_data.size(); ++i) {
                if (input_data[i] > 0) {
                    output_data[i] = input_data[i];
                    mask_data[i] = 1.0;
                } else {
                    output_data[i] = 0.0;
                    mask_data[i] = 0.0;
                }
            }
            
            return output;
        }
        
        Tensor backward(const Tensor& grad_output) {
            if (mask.get_data().empty()) {
                throw std::runtime_error("Must call forward before backward");
            }
            
            Tensor grad_input(grad_output.get_shape(), false);
            
            const auto& grad_out_data = grad_output.get_data();
            const auto& mask_data = mask.get_data();
            auto& grad_in_data = grad_input.get_data();
            
            for (int i = 0; i < grad_out_data.size(); ++i) {
                grad_in_data[i] = grad_out_data[i] * mask_data[i];
            }
            
            return grad_input;
        }
    };

    /**
     * Sigmoid
     * Forward: f(z) = 1 / (1 + exp(-z))
     * Backward: f'(z) = σ(z) * (1 - σ(z))
     */
    class Sigmoid {
    private:
        Tensor output_cache; // Store sigmoid output for backward pass
        
    public:
        Tensor forward(const Tensor& input) {
            Tensor output(input.get_shape(), false);
            output_cache = Tensor(input.get_shape(), false);
            
            const auto& input_data = input.get_data();
            auto& output_data = output.get_data();
            auto& cache_data = output_cache.get_data();
            
            for (int i = 0; i < input_data.size(); ++i) {
                // Numerical stability: use different formulation based on sign
                double z = input_data[i];
                double sigmoid_val;
                
                if (z >= 0) {
                    double exp_neg_z = exp(-z);
                    sigmoid_val = 1.0 / (1.0 + exp_neg_z);
                } else {
                    double exp_z = exp(z);
                    sigmoid_val = exp_z / (1.0 + exp_z);
                }
                
                output_data[i] = sigmoid_val;
                cache_data[i] = sigmoid_val;
            }
            
            return output;
        }
        
        Tensor backward(const Tensor& grad_output) {
            if (output_cache.get_data().empty()) {
                throw std::runtime_error("Must call forward before backward");
            }
            
            Tensor grad_input(grad_output.get_shape(), false);
            
            const auto& grad_out_data = grad_output.get_data();
            const auto& cache_data = output_cache.get_data();
            auto& grad_in_data = grad_input.get_data();
            
            for (int i = 0; i < grad_out_data.size(); ++i) {
                double sigmoid_val = cache_data[i];
                grad_in_data[i] = grad_out_data[i] * sigmoid_val * (1.0 - sigmoid_val);
            }
            
            return grad_input;
        }
    };

    /**
     * Tanh (Hyperbolic Tangent)
     * Forward: f(z) = tanh(z)
     * Backward: f'(z) = 1 - tanh²(z)
     */
    class Tanh {
    private:
        Tensor output_cache; // Store tanh output for backward pass
        
    public:
        Tensor forward(const Tensor& input) {
            Tensor output(input.get_shape(), false);
            output_cache = Tensor(input.get_shape(), false);
            
            const auto& input_data = input.get_data();
            auto& output_data = output.get_data();
            auto& cache_data = output_cache.get_data();
            
            for (int i = 0; i < input_data.size(); ++i) {
                double tanh_val = tanh(input_data[i]);
                output_data[i] = tanh_val;
                cache_data[i] = tanh_val;
            }
            
            return output;
        }
        
        Tensor backward(const Tensor& grad_output) {
            if (output_cache.get_data().empty()) {
                throw std::runtime_error("Must call forward before backward");
            }
            
            Tensor grad_input(grad_output.get_shape(), false);
            
            const auto& grad_out_data = grad_output.get_data();
            const auto& cache_data = output_cache.get_data();
            auto& grad_in_data = grad_input.get_data();
            
            for (int i = 0; i < grad_out_data.size(); ++i) {
                double tanh_val = cache_data[i];
                grad_in_data[i] = grad_out_data[i] * (1.0 - tanh_val * tanh_val);
            }
            
            return grad_input;
        }
    };

    /**
     * Softmax
     * Forward: softmax(z)_i = exp(z_i) / Σ_j exp(z_j)
     * Backward: ∂softmax_i/∂z_j = softmax_i * (δ_ij - softmax_j)
     * 
     * Note: This is typically used with cross-entropy loss which simplifies the gradient
     */
    class Softmax {
    private:
        Tensor output_cache; // Store softmax output for backward pass
        
        // Numerically stable log-sum-exp
        double log_sum_exp(const std::vector<double>& values, int start_idx, int length) {
            double max_val = *std::max_element(values.begin() + start_idx, 
                                             values.begin() + start_idx + length);
            
            double sum = 0.0;
            for (int i = start_idx; i < start_idx + length; ++i) {
                sum += exp(values[i] - max_val);
            }
            
            return max_val + log(sum);
        }
        
    public:
        /**
         * Forward pass for softmax
         * Input shape: [..., num_classes] - applies softmax along last dimension
         */
        Tensor forward(const Tensor& input) {
            if (input.get_shape().empty()) {
                throw std::invalid_argument("Input tensor cannot be empty");
            }
            
            Tensor output(input.get_shape(), false);
            output_cache = Tensor(input.get_shape(), false);
            
            const auto& input_data = input.get_data();
            auto& output_data = output.get_data();
            auto& cache_data = output_cache.get_data();
            
            const auto& shape = input.get_shape();
            int num_classes = shape.back();
            int batch_size = input_data.size() / num_classes;
            
            for (int b = 0; b < batch_size; ++b) {
                int start_idx = b * num_classes;
                
                // Find max for numerical stability
                double max_val = *std::max_element(input_data.begin() + start_idx,
                                                 input_data.begin() + start_idx + num_classes);
                
                // Compute exp(z_i - max) and sum
                double sum_exp = 0.0;
                for (int i = 0; i < num_classes; ++i) {
                    double exp_val = exp(input_data[start_idx + i] - max_val);
                    output_data[start_idx + i] = exp_val;
                    sum_exp += exp_val;
                }
                
                // Normalize
                for (int i = 0; i < num_classes; ++i) {
                    output_data[start_idx + i] /= sum_exp;
                    cache_data[start_idx + i] = output_data[start_idx + i];
                }
            }
            
            return output;
        }
        
        /**
         * Backward pass for softmax
         * Returns full Jacobian multiplication with grad_output
         */
        Tensor backward(const Tensor& grad_output) {
            if (output_cache.get_data().empty()) {
                throw std::runtime_error("Must call forward before backward");
            }
            
            Tensor grad_input(grad_output.get_shape(), false);
            
            const auto& grad_out_data = grad_output.get_data();
            const auto& cache_data = output_cache.get_data();
            auto& grad_in_data = grad_input.get_data();
            
            const auto& shape = grad_output.get_shape();
            int num_classes = shape.back();
            int batch_size = grad_out_data.size() / num_classes;
            
            for (int b = 0; b < batch_size; ++b) {
                int start_idx = b * num_classes;
                
                // Compute Jacobian-vector product
                for (int i = 0; i < num_classes; ++i) {
                    double grad_sum = 0.0;
                    
                    // ∂softmax_i/∂z_j = softmax_i * (δ_ij - softmax_j)
                    for (int j = 0; j < num_classes; ++j) {
                        double jacobian_ij;
                        if (i == j) {
                            jacobian_ij = cache_data[start_idx + i] * (1.0 - cache_data[start_idx + j]);
                        } else {
                            jacobian_ij = -cache_data[start_idx + i] * cache_data[start_idx + j];
                        }
                        grad_sum += grad_out_data[start_idx + j] * jacobian_ij;
                    }
                    
                    grad_in_data[start_idx + i] = grad_sum;
                }
            }
            
            return grad_input;
        }
        
        /**
         * Combined softmax + cross-entropy backward pass
         * This is the common case and has a much simpler gradient: softmax(z) - y
         */
        Tensor backward_with_cross_entropy(const Tensor& softmax_output, const Tensor& targets) {
            if (softmax_output.get_shape() != targets.get_shape()) {
                throw std::invalid_argument("Softmax output and targets must have same shape");
            }
            
            Tensor grad_input(softmax_output.get_shape(), false);
            
            const auto& softmax_data = softmax_output.get_data();
            const auto& target_data = targets.get_data();
            auto& grad_data = grad_input.get_data();
            
            for (int i = 0; i < softmax_data.size(); ++i) {
                grad_data[i] = softmax_data[i] - target_data[i];
            }
            
            return grad_input;
        }
    };

    /**
     * Leaky ReLU
     * Forward: f(z) = max(αz, z) where α is small (e.g., 0.01)
     * Backward: f'(z) = 1 if z > 0, else α
     */
    class LeakyReLU {
    private:
        double alpha;
        Tensor mask; // Store mask for backward pass
        
    public:
        LeakyReLU(double alpha = 0.01) : alpha(alpha) {}
        
        Tensor forward(const Tensor& input) {
            Tensor output(input.get_shape(), false);
            mask = Tensor(input.get_shape(), false);
            
            const auto& input_data = input.get_data();
            auto& output_data = output.get_data();
            auto& mask_data = mask.get_data();
            
            for (int i = 0; i < input_data.size(); ++i) {
                if (input_data[i] > 0) {
                    output_data[i] = input_data[i];
                    mask_data[i] = 1.0;
                } else {
                    output_data[i] = alpha * input_data[i];
                    mask_data[i] = alpha;
                }
            }
            
            return output;
        }
        
        Tensor backward(const Tensor& grad_output) {
            if (mask.get_data().empty()) {
                throw std::runtime_error("Must call forward before backward");
            }
            
            Tensor grad_input(grad_output.get_shape(), false);
            
            const auto& grad_out_data = grad_output.get_data();
            const auto& mask_data = mask.get_data();
            auto& grad_in_data = grad_input.get_data();
            
            for (int i = 0; i < grad_out_data.size(); ++i) {
                grad_in_data[i] = grad_out_data[i] * mask_data[i];
            }
            
            return grad_input;
        }
    };

    /**
     * ELU (Exponential Linear Unit)
     * Forward: f(z) = z if z > 0, else α(exp(z) - 1)
     * Backward: f'(z) = 1 if z > 0, else α*exp(z)
     */
    class ELU {
    private:
        double alpha;
        Tensor input_cache; // Store input for backward pass
        
    public:
        ELU(double alpha = 1.0) : alpha(alpha) {}
        
        Tensor forward(const Tensor& input) {
            Tensor output(input.get_shape(), false);
            input_cache = input.clone();
            
            const auto& input_data = input.get_data();
            auto& output_data = output.get_data();
            
            for (int i = 0; i < input_data.size(); ++i) {
                if (input_data[i] > 0) {
                    output_data[i] = input_data[i];
                } else {
                    output_data[i] = alpha * (exp(input_data[i]) - 1.0);
                }
            }
            
            return output;
        }
        
        Tensor backward(const Tensor& grad_output) {
            if (input_cache.get_data().empty()) {
                throw std::runtime_error("Must call forward before backward");
            }
            
            Tensor grad_input(grad_output.get_shape(), false);
            
            const auto& grad_out_data = grad_output.get_data();
            const auto& input_data = input_cache.get_data();
            auto& grad_in_data = grad_input.get_data();
            
            for (int i = 0; i < grad_out_data.size(); ++i) {
                if (input_data[i] > 0) {
                    grad_in_data[i] = grad_out_data[i];
                } else {
                    grad_in_data[i] = grad_out_data[i] * alpha * exp(input_data[i]);
                }
            }
            
            return grad_input;
        }
    };

    /**
     * GELU (Gaussian Error Linear Unit)
     * Forward: f(z) = z * Φ(z) where Φ is the CDF of standard normal
     * Approximation: f(z) ≈ 0.5 * z * (1 + tanh(√(2/π) * (z + 0.044715 * z³)))
     */
    class GELU {
    private:
        Tensor input_cache; // Store input for backward pass
        static constexpr double SQRT_2_PI = 0.7978845608028654; // sqrt(2/π)
        static constexpr double COEFF = 0.044715;
        
    public:
        Tensor forward(const Tensor& input) {
            Tensor output(input.get_shape(), false);
            input_cache = input.clone();
            
            const auto& input_data = input.get_data();
            auto& output_data = output.get_data();
            
            for (int i = 0; i < input_data.size(); ++i) {
                double z = input_data[i];
                double z_cubed = z * z * z;
                double tanh_arg = SQRT_2_PI * (z + COEFF * z_cubed);
                output_data[i] = 0.5 * z * (1.0 + tanh(tanh_arg));
            }
            
            return output;
        }
        
        Tensor backward(const Tensor& grad_output) {
            if (input_cache.get_data().empty()) {
                throw std::runtime_error("Must call forward before backward");
            }
            
            Tensor grad_input(grad_output.get_shape(), false);
            
            const auto& grad_out_data = grad_output.get_data();
            const auto& input_data = input_cache.get_data();
            auto& grad_in_data = grad_input.get_data();
            
            for (int i = 0; i < grad_out_data.size(); ++i) {
                double z = input_data[i];
                double z_squared = z * z;
                double z_cubed = z_squared * z;
                
                double tanh_arg = SQRT_2_PI * (z + COEFF * z_cubed);
                double tanh_val = tanh(tanh_arg);
                double sech_squared = 1.0 - tanh_val * tanh_val; // sech²(x) = 1 - tanh²(x)
                
                double dtanh_dz = SQRT_2_PI * (1.0 + 3.0 * COEFF * z_squared);
                
                double dgelu_dz = 0.5 * (1.0 + tanh_val) + 0.5 * z * sech_squared * dtanh_dz;
                
                grad_in_data[i] = grad_out_data[i] * dgelu_dz;
            }
            
            return grad_input;
        }
    };

    // Utility function to apply activation in-place
    template<typename ActivationFunc>
    void apply_activation_inplace(Tensor& tensor, ActivationFunc& activation) {
        Tensor result = activation.forward(tensor);
        tensor.get_data() = result.get_data();
    }

} // namespace ActivationFunctions

#endif // ACTIVATION_FUNCTIONS_HPP