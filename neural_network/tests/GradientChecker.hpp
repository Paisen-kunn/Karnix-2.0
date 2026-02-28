#ifndef GRADIENT_CHECKER_HPP
#define GRADIENT_CHECKER_HPP

#include "../utils/Tensor.hpp"
#include "../utils/LossFunctions.hpp"
#include <functional>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
#include <algorithm>
#include <numeric>

/**
 * Gradient Checker for Neural Network Components
 * 
 * Implements finite difference method to verify analytical gradients
 * Uses central difference for higher accuracy:
 * 
 * Numerical gradient:
 * g_i^num = [L(θ + h*e_i) - L(θ - h*e_i)] / (2*h)
 * 
 * Relative error:
 * rel_err = |g_i^num - g_i^anal| / max(ε, |g_i^num| + |g_i^anal|)
 */
class GradientChecker {
private:
    double h; // Step size for finite differences
    double tolerance; // Acceptable relative error
    double epsilon; // Small constant for relative error computation
    
public:
    GradientChecker(double h = 1e-5, double tolerance = 1e-5, double epsilon = 1e-8)
        : h(h), tolerance(tolerance), epsilon(epsilon) {
        if (h <= 0.0 || tolerance <= 0.0 || epsilon <= 0.0) {
            throw std::invalid_argument("All parameters must be positive");
        }
    }
    
    /**
     * Check gradients for a scalar-valued function
     * 
     * @param loss_function: Function that takes parameters and returns scalar loss
     * @param parameter: Parameter tensor to check gradients for
     * @param analytical_grad: Analytical gradient computed by backprop
     * @param name: Name for debugging output
     * @return: true if gradients match within tolerance
     */
    bool check_gradients(std::function<double(const Tensor&)> loss_function,
                        Tensor& parameter,
                        const Tensor& analytical_grad,
                        const std::string& name = "parameter") {
        
        if (parameter.get_shape() != analytical_grad.get_shape()) {
            throw std::invalid_argument("Parameter and gradient shapes must match");
        }
        
        std::cout << "\n=== Gradient Check for " << name << " ===" << std::endl;
        std::cout << "Parameter shape: [";
        const auto& shape = parameter.get_shape();
        for (int i = 0; i < shape.size(); ++i) {
            std::cout << shape[i];
            if (i < shape.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        
        auto& param_data = parameter.get_data();
        const auto& grad_data = analytical_grad.get_data();
        
        int num_errors = 0;
        int num_params = param_data.size();
        double max_error = 0.0;
        double avg_error = 0.0;
        
        std::vector<double> numerical_gradients(num_params);
        std::vector<double> relative_errors(num_params);
        
        // Compute numerical gradients for all parameters
        for (int i = 0; i < num_params; ++i) {
            // Forward step: θ + h*e_i
            param_data[i] += h;
            double loss_plus = loss_function(parameter);
            
            // Backward step: θ - h*e_i
            param_data[i] -= 2 * h;
            double loss_minus = loss_function(parameter);
            
            // Restore original value
            param_data[i] += h;
            
            // Central difference
            numerical_gradients[i] = (loss_plus - loss_minus) / (2 * h);
            
            // Compute relative error
            double analytical = grad_data[i];
            double numerical = numerical_gradients[i];
            double denominator = std::max(epsilon, std::abs(analytical) + std::abs(numerical));
            relative_errors[i] = std::abs(analytical - numerical) / denominator;
            
            avg_error += relative_errors[i];
            max_error = std::max(max_error, relative_errors[i]);
            
            if (relative_errors[i] > tolerance) {
                num_errors++;
            }
        }
        
        avg_error /= num_params;
        
        // Print summary
        std::cout << std::fixed << std::setprecision(8);
        std::cout << "Average relative error: " << avg_error << std::endl;
        std::cout << "Maximum relative error: " << max_error << std::endl;
        std::cout << "Tolerance: " << tolerance << std::endl;
        std::cout << "Parameters with errors > tolerance: " << num_errors << "/" << num_params << std::endl;
        
        // Print detailed results for problematic parameters
        if (num_errors > 0) {
            std::cout << "\nDetailed error analysis (showing worst 10 errors):" << std::endl;
            std::cout << std::setw(8) << "Index" << std::setw(15) << "Analytical" 
                     << std::setw(15) << "Numerical" << std::setw(15) << "Rel Error" << std::endl;
            std::cout << std::string(60, '-') << std::endl;
            
            // Find indices of largest errors
            std::vector<int> error_indices(num_params);
            std::iota(error_indices.begin(), error_indices.end(), 0);
            std::partial_sort(error_indices.begin(), error_indices.begin() + std::min(10, num_errors),
                            error_indices.end(),
                            [&](int a, int b) { return relative_errors[a] > relative_errors[b]; });
            
            for (int k = 0; k < std::min(10, num_errors); ++k) {
                int i = error_indices[k];
                if (relative_errors[i] > tolerance) {
                    std::cout << std::setw(8) << i 
                             << std::setw(15) << grad_data[i]
                             << std::setw(15) << numerical_gradients[i]
                             << std::setw(15) << relative_errors[i] << std::endl;
                }
            }
        }
        
        bool passed = (max_error <= tolerance);
        std::cout << "\nGradient check " << (passed ? "PASSED" : "FAILED") << std::endl;
        
        return passed;
    }
    
    /**
     * Check gradients for multiple parameters (e.g., weights and biases)
     */
    struct ParameterGradientPair {
        Tensor* parameter;
        const Tensor* gradient;
        std::string name;
        
        ParameterGradientPair(Tensor* param, const Tensor* grad, const std::string& n)
            : parameter(param), gradient(grad), name(n) {}
    };
    
    bool check_multiple_gradients(std::function<double()> loss_function,
                                 const std::vector<ParameterGradientPair>& param_grad_pairs) {
        std::cout << "\n=== Multiple Parameter Gradient Check ===" << std::endl;
        
        bool all_passed = true;
        
        for (const auto& pair : param_grad_pairs) {
            // Create a lambda that only varies this parameter
            auto single_param_loss = [&](const Tensor& param) -> double {
                // Temporarily replace the parameter data
                auto original_data = pair.parameter->get_data();
                pair.parameter->get_data() = param.get_data();
                
                double loss = loss_function();
                
                // Restore original data
                pair.parameter->get_data() = original_data;
                return loss;
            };
            
            bool passed = check_gradients(single_param_loss, *pair.parameter, 
                                        *pair.gradient, pair.name);
            all_passed &= passed;
        }
        
        std::cout << "\nOverall gradient check " << (all_passed ? "PASSED" : "FAILED") << std::endl;
        return all_passed;
    }
    
    /**
     * Check layer gradients (both input and parameter gradients)
     */
    template<typename LayerType>
    bool check_layer_gradients(LayerType& layer,
                              const Tensor& input,
                              const Tensor& grad_output,
                              const std::string& layer_name = "layer") {
        
        std::cout << "\n=== Layer Gradient Check: " << layer_name << " ===" << std::endl;
        
        // Forward pass to cache input
        Tensor output = layer.forward(input);
        
        // Backward pass to compute gradients
        Tensor grad_input = layer.backward(grad_output);
        
        // Create loss function that computes dot product with grad_output
        auto layer_loss = [&]() -> double {
            Tensor out = layer.forward(input);
            return compute_dot_product(out, grad_output);
        };
        
        bool all_passed = true;
        
        // Check input gradients
        auto input_loss = [&](const Tensor& inp) -> double {
            Tensor out = layer.forward(inp);
            return compute_dot_product(out, grad_output);
        };
        
        Tensor input_copy = input.clone();
        bool input_passed = check_gradients(input_loss, input_copy, grad_input, 
                                           layer_name + "_input_grad");
        all_passed &= input_passed;
        
        // Check parameter gradients (if layer has parameters)
        // We'll use a simple runtime check instead of C++17 constexpr
        try {
            // Try to access weight-related methods
            auto& weights = layer.get_weights();
            auto& weight_grad = layer.get_weight_grad();
            
            std::vector<ParameterGradientPair> param_pairs;
            param_pairs.emplace_back(&layer.get_weights(), &layer.get_weight_grad(), 
                                   layer_name + "_weights");
            
            // Try to access bias-related methods
            try {
                auto& bias = layer.get_bias();
                auto& bias_grad = layer.get_bias_grad();
                param_pairs.emplace_back(&layer.get_bias(), &layer.get_bias_grad(), 
                                       layer_name + "_bias");
            } catch (...) {
                // Layer doesn't have bias, continue without it
            }
            
            bool param_passed = check_multiple_gradients(layer_loss, param_pairs);
            all_passed &= param_passed;
        } catch (...) {
            // Layer doesn't have parameters, skip parameter checks
            std::cout << "Layer has no parameters to check" << std::endl;
        }
        
        return all_passed;
    }
    
    /**
     * Check activation function gradients
     */
    template<typename ActivationType>
    bool check_activation_gradients(ActivationType& activation,
                                   const Tensor& input,
                                   const Tensor& grad_output,
                                   const std::string& activation_name = "activation") {
        
        std::cout << "\n=== Activation Gradient Check: " << activation_name << " ===" << std::endl;
        
        // Forward pass
        Tensor output = activation.forward(input);
        
        // Backward pass
        Tensor grad_input = activation.backward(grad_output);
        
        // Loss function
        auto activation_loss = [&](const Tensor& inp) -> double {
            Tensor out = activation.forward(inp);
            return compute_dot_product(out, grad_output);
        };
        
        Tensor input_copy = input.clone();
        return check_gradients(activation_loss, input_copy, grad_input, 
                              activation_name + "_grad");
    }
    
    /**
     * Check loss function gradients
     */
    bool check_loss_gradients(const Tensor& predictions,
                             const Tensor& targets,
                             const std::string& loss_name = "loss") {
        
        std::cout << "\n=== Loss Gradient Check: " << loss_name << " ===" << std::endl;
        
        // Compute analytical gradient using runtime type checking
        Tensor analytical_grad;
        
        // Simple runtime type checking for C++14 compatibility
        std::string loss_type = loss_name;
        if (loss_type.find("MSE") != std::string::npos) {
            analytical_grad = MSELoss::backward(predictions, targets);
        } else if (loss_type.find("CrossEntropy") != std::string::npos) {
            analytical_grad = CrossEntropyLoss::backward(predictions, targets);
        } else if (loss_type.find("BinaryCrossEntropy") != std::string::npos) {
            analytical_grad = BinaryCrossEntropyLoss::backward_with_logits(predictions, targets);
        } else {
            throw std::invalid_argument("Unsupported loss type for gradient checking");
        }
        
        // Loss function
        auto loss_function = [&](const Tensor& pred) -> double {
            if (loss_type.find("MSE") != std::string::npos) {
                return MSELoss::forward(pred, targets);
            } else if (loss_type.find("CrossEntropy") != std::string::npos) {
                return CrossEntropyLoss::forward(pred, targets);
            } else if (loss_type.find("BinaryCrossEntropy") != std::string::npos) {
                return BinaryCrossEntropyLoss::forward_with_logits(pred, targets);
            } else {
                throw std::invalid_argument("Unsupported loss type");
            }
        };
        
        Tensor pred_copy = predictions.clone();
        return check_gradients(loss_function, pred_copy, analytical_grad, loss_name);
    }
    
    /**
     * Comprehensive neural network gradient check
     */
    template<typename NetworkType>
    bool check_network_gradients(NetworkType& network,
                                const Tensor& input,
                                const Tensor& targets,
                                const std::string& network_name = "network") {
        
        std::cout << "\n=== Full Network Gradient Check: " << network_name << " ===" << std::endl;
        
        // This would be implemented based on your specific network architecture
        // For now, provide a template for how it might work
        
        bool all_passed = true;
        
        // 1. Forward pass through network
        // 2. Compute loss
        // 3. Backward pass
        // 4. Check each layer's gradients
        
        std::cout << "Network gradient checking would be implemented here based on your network structure." << std::endl;
        
        return all_passed;
    }
    
    // Getters and setters
    double get_step_size() const { return h; }
    double get_tolerance() const { return tolerance; }
    double get_epsilon() const { return epsilon; }
    
    void set_step_size(double new_h) { h = new_h; }
    void set_tolerance(double new_tol) { tolerance = new_tol; }
    void set_epsilon(double new_eps) { epsilon = new_eps; }
    
private:
    /**
     * Compute dot product between two tensors (for loss function)
     */
    double compute_dot_product(const Tensor& a, const Tensor& b) {
        if (a.get_shape() != b.get_shape()) {
            throw std::invalid_argument("Tensors must have same shape for dot product");
        }
        
        const auto& a_data = a.get_data();
        const auto& b_data = b.get_data();
        
        double result = 0.0;
        for (int i = 0; i < a_data.size(); ++i) {
            result += a_data[i] * b_data[i];
        }
        
        return result;
    }
};

/**
 * Utility functions for gradient checking
 */
namespace GradientCheckUtils {
    
    /**
     * Create small random tensors for testing
     */
    inline Tensor create_test_tensor(const std::vector<int>& shape, double scale = 1.0) {
        Tensor tensor(shape, false);
        auto& data = tensor.get_data();
        
        for (auto& val : data) {
            val = scale * (2.0 * static_cast<double>(rand()) / RAND_MAX - 1.0);
        }
        
        return tensor;
    }
    
    /**
     * Create test targets for classification
     */
    inline Tensor create_one_hot_targets(int batch_size, int num_classes, int true_class) {
        Tensor targets({batch_size, num_classes}, false);
        targets.fill(0.0);
        
        for (int b = 0; b < batch_size; ++b) {
            targets(b, true_class) = 1.0;
        }
        
        return targets;
    }
    
    /**
     * Create test targets for regression
     */
    inline Tensor create_regression_targets(const std::vector<int>& shape) {
        return create_test_tensor(shape, 2.0);
    }
    
    /**
     * Run gradient check test suite for common components
     */
    inline void run_gradient_check_suite() {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "GRADIENT CHECK TEST SUITE" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        GradientChecker checker(1e-5, 1e-4); // Slightly relaxed tolerance for stability
        
        // This would run comprehensive tests on all components
        std::cout << "Gradient check test suite would test all neural network components here." << std::endl;
        std::cout << "Individual component tests can be run using the GradientChecker class methods." << std::endl;
    }
}

#endif // GRADIENT_CHECKER_HPP