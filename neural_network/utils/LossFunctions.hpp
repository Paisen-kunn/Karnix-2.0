#ifndef LOSS_FUNCTIONS_HPP
#define LOSS_FUNCTIONS_HPP

#include "../utils/Tensor.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>

/**
 * Loss Functions with Numerical Stability
 * All functions implement both forward (loss computation) and backward (gradient) passes
 */

/**
 * Mean Squared Error (MSE) Loss
 * 
 * Mathematical Formulation:
 * L = (1/2N) * Σᵢ (yᵢ - ŷᵢ)²
 * 
 * Gradient:
 * ∂L/∂ŷᵢ = (1/N) * (ŷᵢ - yᵢ)
 */
class MSELoss {
public:
    static double forward(const Tensor& predictions, const Tensor& targets) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        
        double total_loss = 0.0;
        for (int i = 0; i < pred_data.size(); ++i) {
            double diff = pred_data[i] - target_data[i];
            total_loss += diff * diff;
        }
        
        return total_loss / (2.0 * pred_data.size());
    }
    
    static Tensor backward(const Tensor& predictions, const Tensor& targets) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        Tensor grad(predictions.get_shape(), false);
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        auto& grad_data = grad.get_data();
        
        double scale = 1.0 / pred_data.size();
        
        for (int i = 0; i < pred_data.size(); ++i) {
            grad_data[i] = scale * (pred_data[i] - target_data[i]);
        }
        
        return grad;
    }
};

/**
 * Mean Absolute Error (MAE) Loss
 * 
 * Mathematical Formulation:
 * L = (1/N) * Σᵢ |yᵢ - ŷᵢ|
 * 
 * Gradient:
 * ∂L/∂ŷᵢ = (1/N) * sign(ŷᵢ - yᵢ)
 */
class MAELoss {
public:
    static double forward(const Tensor& predictions, const Tensor& targets) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        
        double total_loss = 0.0;
        for (int i = 0; i < pred_data.size(); ++i) {
            total_loss += std::abs(pred_data[i] - target_data[i]);
        }
        
        return total_loss / pred_data.size();
    }
    
    static Tensor backward(const Tensor& predictions, const Tensor& targets) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        Tensor grad(predictions.get_shape(), false);
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        auto& grad_data = grad.get_data();
        
        double scale = 1.0 / pred_data.size();
        
        for (int i = 0; i < pred_data.size(); ++i) {
            double diff = pred_data[i] - target_data[i];
            if (diff > 0) {
                grad_data[i] = scale;
            } else if (diff < 0) {
                grad_data[i] = -scale;
            } else {
                grad_data[i] = 0.0; // Subgradient at 0
            }
        }
        
        return grad;
    }
};

/**
 * Huber Loss (Smooth L1 Loss)
 * Combines MSE and MAE for robustness
 * 
 * Mathematical Formulation:
 * L = {
 *   0.5 * (y - ŷ)²     if |y - ŷ| ≤ δ
 *   δ * |y - ŷ| - 0.5 * δ²   otherwise
 * }
 */
class HuberLoss {
private:
    double delta;
    
public:
    HuberLoss(double delta = 1.0) : delta(delta) {
        if (delta <= 0.0) {
            throw std::invalid_argument("Delta must be positive");
        }
    }
    
    double forward(const Tensor& predictions, const Tensor& targets) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        
        double total_loss = 0.0;
        for (int i = 0; i < pred_data.size(); ++i) {
            double diff = std::abs(pred_data[i] - target_data[i]);
            if (diff <= delta) {
                total_loss += 0.5 * diff * diff;
            } else {
                total_loss += delta * diff - 0.5 * delta * delta;
            }
        }
        
        return total_loss / pred_data.size();
    }
    
    Tensor backward(const Tensor& predictions, const Tensor& targets) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        Tensor grad(predictions.get_shape(), false);
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        auto& grad_data = grad.get_data();
        
        double scale = 1.0 / pred_data.size();
        
        for (int i = 0; i < pred_data.size(); ++i) {
            double diff = pred_data[i] - target_data[i];
            double abs_diff = std::abs(diff);
            
            if (abs_diff <= delta) {
                grad_data[i] = scale * diff;
            } else {
                grad_data[i] = scale * delta * (diff > 0 ? 1.0 : -1.0);
            }
        }
        
        return grad;
    }
};

/**
 * Cross-Entropy Loss with Numerical Stability
 * 
 * Mathematical Formulation:
 * L = -Σᵢ yᵢ * log(ŷᵢ)
 * 
 * For numerical stability, we use log-sum-exp trick:
 * log(softmax(zᵢ)) = zᵢ - log(Σⱼ exp(zⱼ)) = zᵢ - log_sum_exp(z)
 */
class CrossEntropyLoss {
private:
    // Numerically stable log-sum-exp
    static double log_sum_exp(const std::vector<double>& logits, int start_idx, int length) {
        double max_val = *std::max_element(logits.begin() + start_idx, 
                                         logits.begin() + start_idx + length);
        
        double sum = 0.0;
        for (int i = start_idx; i < start_idx + length; ++i) {
            sum += exp(logits[i] - max_val);
        }
        
        return max_val + log(sum);
    }
    
public:
    /**
     * Forward pass
     * logits: raw scores before softmax [batch_size, num_classes] or [num_classes]
     * targets: one-hot encoded or class indices [batch_size, num_classes] or [num_classes]
     */
    static double forward(const Tensor& logits, const Tensor& targets) {
        if (logits.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Logits and targets must have same shape");
        }
        
        const auto& logit_data = logits.get_data();
        const auto& target_data = targets.get_data();
        const auto& shape = logits.get_shape();
        
        if (logits.get_dims() == 1) {
            // Single sample
            int num_classes = shape[0];
            double log_sum_exp_val = log_sum_exp(logit_data, 0, num_classes);
            
            double loss = 0.0;
            for (int i = 0; i < num_classes; ++i) {
                if (target_data[i] > 0) { // Only compute for positive targets
                    double log_prob = logit_data[i] - log_sum_exp_val;
                    loss -= target_data[i] * log_prob;
                }
            }
            return loss;
            
        } else if (logits.get_dims() == 2) {
            // Batch
            int batch_size = shape[0];
            int num_classes = shape[1];
            
            double total_loss = 0.0;
            for (int b = 0; b < batch_size; ++b) {
                int start_idx = b * num_classes;
                double log_sum_exp_val = log_sum_exp(logit_data, start_idx, num_classes);
                
                for (int i = 0; i < num_classes; ++i) {
                    int idx = start_idx + i;
                    if (target_data[idx] > 0) {
                        double log_prob = logit_data[idx] - log_sum_exp_val;
                        total_loss -= target_data[idx] * log_prob;
                    }
                }
            }
            return total_loss / batch_size;
            
        } else {
            throw std::invalid_argument("Logits must be 1D or 2D");
        }
    }
    
    /**
     * Backward pass
     * Returns gradient w.r.t. logits
     * 
     * The gradient is simply: softmax(logits) - targets
     * This is the famous result from combining softmax + cross-entropy
     */
    static Tensor backward(const Tensor& logits, const Tensor& targets) {
        if (logits.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Logits and targets must have same shape");
        }
        
        // Compute softmax
        Tensor softmax_output = compute_softmax(logits);
        
        // Gradient: softmax - targets
        Tensor grad(logits.get_shape(), false);
        
        const auto& softmax_data = softmax_output.get_data();
        const auto& target_data = targets.get_data();
        auto& grad_data = grad.get_data();
        
        const auto& shape = logits.get_shape();
        double scale = (logits.get_dims() == 2) ? 1.0 / shape[0] : 1.0; // Average over batch
        
        for (int i = 0; i < grad_data.size(); ++i) {
            grad_data[i] = scale * (softmax_data[i] - target_data[i]);
        }
        
        return grad;
    }
    
private:
    /**
     * Compute softmax with numerical stability
     */
    static Tensor compute_softmax(const Tensor& logits) {
        Tensor softmax(logits.get_shape(), false);
        
        const auto& logit_data = logits.get_data();
        auto& softmax_data = softmax.get_data();
        const auto& shape = logits.get_shape();
        
        if (logits.get_dims() == 1) {
            // Single sample
            int num_classes = shape[0];
            double log_sum_exp_val = log_sum_exp(logit_data, 0, num_classes);
            
            for (int i = 0; i < num_classes; ++i) {
                softmax_data[i] = exp(logit_data[i] - log_sum_exp_val);
            }
            
        } else if (logits.get_dims() == 2) {
            // Batch
            int batch_size = shape[0];
            int num_classes = shape[1];
            
            for (int b = 0; b < batch_size; ++b) {
                int start_idx = b * num_classes;
                double log_sum_exp_val = log_sum_exp(logit_data, start_idx, num_classes);
                
                for (int i = 0; i < num_classes; ++i) {
                    int idx = start_idx + i;
                    softmax_data[idx] = exp(logit_data[idx] - log_sum_exp_val);
                }
            }
        }
        
        return softmax;
    }
};

/**
 * Binary Cross-Entropy Loss
 * 
 * Mathematical Formulation:
 * L = -[y * log(ŷ) + (1-y) * log(1-ŷ)]
 * 
 * For numerical stability, we use logits directly:
 * L = max(z, 0) - z*y + log(1 + exp(-|z|))
 * where z are the logits before sigmoid
 */
class BinaryCrossEntropyLoss {
public:
    /**
     * Forward pass with logits (more numerically stable)
     */
    static double forward_with_logits(const Tensor& logits, const Tensor& targets) {
        if (logits.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Logits and targets must have same shape");
        }
        
        const auto& logit_data = logits.get_data();
        const auto& target_data = targets.get_data();
        
        double total_loss = 0.0;
        for (int i = 0; i < logit_data.size(); ++i) {
            double z = logit_data[i];
            double y = target_data[i];
            
            // Numerically stable BCE: max(z, 0) - z*y + log(1 + exp(-|z|))
            double loss = std::max(z, 0.0) - z * y + log(1.0 + exp(-std::abs(z)));
            total_loss += loss;
        }
        
        return total_loss / logit_data.size();
    }
    
    /**
     * Backward pass with logits
     * Gradient: sigmoid(z) - y
     */
    static Tensor backward_with_logits(const Tensor& logits, const Tensor& targets) {
        if (logits.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Logits and targets must have same shape");
        }
        
        Tensor grad(logits.get_shape(), false);
        
        const auto& logit_data = logits.get_data();
        const auto& target_data = targets.get_data();
        auto& grad_data = grad.get_data();
        
        double scale = 1.0 / logit_data.size();
        
        for (int i = 0; i < logit_data.size(); ++i) {
            double z = logit_data[i];
            double y = target_data[i];
            
            // Compute sigmoid with numerical stability
            double sigmoid_val;
            if (z >= 0) {
                double exp_neg_z = exp(-z);
                sigmoid_val = 1.0 / (1.0 + exp_neg_z);
            } else {
                double exp_z = exp(z);
                sigmoid_val = exp_z / (1.0 + exp_z);
            }
            
            grad_data[i] = scale * (sigmoid_val - y);
        }
        
        return grad;
    }
    
    /**
     * Forward pass with probabilities (less stable, for completeness)
     */
    static double forward(const Tensor& predictions, const Tensor& targets, double epsilon = 1e-7) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        
        double total_loss = 0.0;
        for (int i = 0; i < pred_data.size(); ++i) {
            double p = std::max(epsilon, std::min(1.0 - epsilon, pred_data[i])); // Clamp for stability
            double y = target_data[i];
            
            total_loss -= y * log(p) + (1.0 - y) * log(1.0 - p);
        }
        
        return total_loss / pred_data.size();
    }
};

/**
 * Focal Loss
 * Addresses class imbalance by down-weighting easy examples
 * 
 * Mathematical Formulation:
 * FL(p_t) = -α_t * (1 - p_t)^γ * log(p_t)
 * 
 * Where p_t = p if y=1, else 1-p
 */
class FocalLoss {
private:
    double alpha;
    double gamma;
    
public:
    FocalLoss(double alpha = 1.0, double gamma = 2.0) : alpha(alpha), gamma(gamma) {}
    
    double forward(const Tensor& predictions, const Tensor& targets, double epsilon = 1e-7) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        
        double total_loss = 0.0;
        for (int i = 0; i < pred_data.size(); ++i) {
            double p = std::max(epsilon, std::min(1.0 - epsilon, pred_data[i]));
            double y = target_data[i];
            
            double p_t = y * p + (1.0 - y) * (1.0 - p);
            double alpha_t = y * alpha + (1.0 - y) * (1.0 - alpha);
            
            double focal_weight = alpha_t * pow(1.0 - p_t, gamma);
            total_loss -= focal_weight * log(p_t);
        }
        
        return total_loss / pred_data.size();
    }
    
    Tensor backward(const Tensor& predictions, const Tensor& targets, double epsilon = 1e-7) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        Tensor grad(predictions.get_shape(), false);
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        auto& grad_data = grad.get_data();
        
        double scale = 1.0 / pred_data.size();
        
        for (int i = 0; i < pred_data.size(); ++i) {
            double p = std::max(epsilon, std::min(1.0 - epsilon, pred_data[i]));
            double y = target_data[i];
            
            double p_t = y * p + (1.0 - y) * (1.0 - p);
            double alpha_t = y * alpha + (1.0 - y) * (1.0 - alpha);
            
            // Gradient computation (complex derivative)
            double focal_weight = alpha_t * pow(1.0 - p_t, gamma);
            double grad_focal = alpha_t * gamma * pow(1.0 - p_t, gamma - 1.0);
            
            if (y == 1.0) {
                grad_data[i] = scale * (-focal_weight / p + grad_focal * log(p));
            } else {
                grad_data[i] = scale * (focal_weight / (1.0 - p) - grad_focal * log(1.0 - p));
            }
        }
        
        return grad;
    }
};

/**
 * KL Divergence Loss
 * Measures difference between two probability distributions
 * 
 * Mathematical Formulation:
 * KL(P||Q) = Σᵢ P(i) * log(P(i) / Q(i))
 */
class KLDivergenceLoss {
public:
    static double forward(const Tensor& predictions, const Tensor& targets, double epsilon = 1e-7) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        
        double total_loss = 0.0;
        for (int i = 0; i < pred_data.size(); ++i) {
            double q = std::max(epsilon, pred_data[i]); // Predicted probability
            double p = std::max(epsilon, target_data[i]); // True probability
            
            total_loss += p * log(p / q);
        }
        
        return total_loss;
    }
    
    static Tensor backward(const Tensor& predictions, const Tensor& targets, double epsilon = 1e-7) {
        if (predictions.get_shape() != targets.get_shape()) {
            throw std::invalid_argument("Predictions and targets must have same shape");
        }
        
        Tensor grad(predictions.get_shape(), false);
        
        const auto& pred_data = predictions.get_data();
        const auto& target_data = targets.get_data();
        auto& grad_data = grad.get_data();
        
        for (int i = 0; i < pred_data.size(); ++i) {
            double q = std::max(epsilon, pred_data[i]);
            double p = std::max(epsilon, target_data[i]);
            
            grad_data[i] = -p / q;
        }
        
        return grad;
    }
};

#endif // LOSS_FUNCTIONS_HPP