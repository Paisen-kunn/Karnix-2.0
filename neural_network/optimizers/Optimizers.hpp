#ifndef OPTIMIZERS_HPP
#define OPTIMIZERS_HPP

#include "../utils/Tensor.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <iostream>

/**
 * Base Optimizer Class
 * Defines interface for all optimizers
 */
class Optimizer {
public:
    virtual ~Optimizer() = default;
    virtual void update(Tensor& parameter, const Tensor& gradient) = 0;
    virtual void zero_grad() {} // Some optimizers may need to clear state
    virtual void print_info() const = 0;
    virtual std::unique_ptr<Optimizer> clone() const = 0;
};

/**
 * SGD (Stochastic Gradient Descent)
 * 
 * Mathematical Formulation:
 * θ_{t+1} = θ_t - η * g_t
 * 
 * Where:
 * - θ_t: parameters at time t
 * - η: learning rate
 * - g_t: gradient at time t
 */
class SGD : public Optimizer {
private:
    double learning_rate;
    
public:
    SGD(double learning_rate = 0.01) : learning_rate(learning_rate) {
        if (learning_rate <= 0.0) {
            throw std::invalid_argument("Learning rate must be positive");
        }
    }
    
    void update(Tensor& parameter, const Tensor& gradient) override {
        if (parameter.get_shape() != gradient.get_shape()) {
            throw std::invalid_argument("Parameter and gradient shapes must match");
        }
        
        auto& param_data = parameter.get_data();
        const auto& grad_data = gradient.get_data();
        
        for (int i = 0; i < param_data.size(); ++i) {
            param_data[i] -= learning_rate * grad_data[i];
        }
    }
    
    void print_info() const override {
        std::cout << "SGD Optimizer:" << std::endl;
        std::cout << "  Learning rate: " << learning_rate << std::endl;
    }
    
    std::unique_ptr<Optimizer> clone() const override {
        return std::make_unique<SGD>(learning_rate);
    }
    
    double get_learning_rate() const { return learning_rate; }
    void set_learning_rate(double lr) { learning_rate = lr; }
};

/**
 * SGD with Momentum
 * 
 * Mathematical Formulation:
 * v_{t+1} = μ * v_t + η * g_t
 * θ_{t+1} = θ_t - v_{t+1}
 * 
 * Where:
 * - μ: momentum coefficient (typically 0.9)
 * - v_t: velocity (momentum) at time t
 */
class SGDMomentum : public Optimizer {
private:
    double learning_rate;
    double momentum;
    std::unordered_map<void*, Tensor> velocities; // Map parameter address to velocity
    
public:
    SGDMomentum(double learning_rate = 0.01, double momentum = 0.9) 
        : learning_rate(learning_rate), momentum(momentum) {
        if (learning_rate <= 0.0) {
            throw std::invalid_argument("Learning rate must be positive");
        }
        if (momentum < 0.0 || momentum >= 1.0) {
            throw std::invalid_argument("Momentum must be in [0, 1)");
        }
    }
    
    void update(Tensor& parameter, const Tensor& gradient) override {
        if (parameter.get_shape() != gradient.get_shape()) {
            throw std::invalid_argument("Parameter and gradient shapes must match");
        }
        
        void* param_ptr = &parameter;
        
        // Initialize velocity if this is the first time seeing this parameter
        if (velocities.find(param_ptr) == velocities.end()) {
            velocities[param_ptr] = Tensor(parameter.get_shape(), false);
            velocities[param_ptr].fill(0.0);
        }
        
        auto& velocity = velocities[param_ptr];
        auto& param_data = parameter.get_data();
        const auto& grad_data = gradient.get_data();
        auto& vel_data = velocity.get_data();
        
        // Update velocity and parameters
        for (int i = 0; i < param_data.size(); ++i) {
            vel_data[i] = momentum * vel_data[i] + learning_rate * grad_data[i];
            param_data[i] -= vel_data[i];
        }
    }
    
    void zero_grad() override {
        // Optionally clear velocities (uncommon, but sometimes useful)
        // velocities.clear();
    }
    
    void print_info() const override {
        std::cout << "SGD with Momentum Optimizer:" << std::endl;
        std::cout << "  Learning rate: " << learning_rate << std::endl;
        std::cout << "  Momentum: " << momentum << std::endl;
        std::cout << "  Tracked parameters: " << velocities.size() << std::endl;
    }
    
    std::unique_ptr<Optimizer> clone() const override {
        return std::make_unique<SGDMomentum>(learning_rate, momentum);
    }
    
    double get_learning_rate() const { return learning_rate; }
    double get_momentum() const { return momentum; }
};

/**
 * RMSProp Optimizer
 * 
 * Mathematical Formulation:
 * s_t = ρ * s_{t-1} + (1 - ρ) * g_t²
 * θ_{t+1} = θ_t - η * g_t / (√(s_t) + ε)
 * 
 * Where:
 * - ρ: decay rate (typically 0.9)
 * - s_t: moving average of squared gradients
 * - ε: small constant for numerical stability (typically 1e-8)
 */
class RMSProp : public Optimizer {
private:
    double learning_rate;
    double rho; // decay rate
    double epsilon;
    std::unordered_map<void*, Tensor> squared_grads; // Map parameter address to s_t
    
public:
    RMSProp(double learning_rate = 0.001, double rho = 0.9, double epsilon = 1e-8)
        : learning_rate(learning_rate), rho(rho), epsilon(epsilon) {
        if (learning_rate <= 0.0) {
            throw std::invalid_argument("Learning rate must be positive");
        }
        if (rho < 0.0 || rho >= 1.0) {
            throw std::invalid_argument("Rho must be in [0, 1)");
        }
        if (epsilon <= 0.0) {
            throw std::invalid_argument("Epsilon must be positive");
        }
    }
    
    void update(Tensor& parameter, const Tensor& gradient) override {
        if (parameter.get_shape() != gradient.get_shape()) {
            throw std::invalid_argument("Parameter and gradient shapes must match");
        }
        
        void* param_ptr = &parameter;
        
        // Initialize squared gradients if this is the first time seeing this parameter
        if (squared_grads.find(param_ptr) == squared_grads.end()) {
            squared_grads[param_ptr] = Tensor(parameter.get_shape(), false);
            squared_grads[param_ptr].fill(0.0);
        }
        
        auto& s_t = squared_grads[param_ptr];
        auto& param_data = parameter.get_data();
        const auto& grad_data = gradient.get_data();
        auto& s_data = s_t.get_data();
        
        // Update moving average of squared gradients and parameters
        for (int i = 0; i < param_data.size(); ++i) {
            // s_t = ρ * s_{t-1} + (1 - ρ) * g_t²
            s_data[i] = rho * s_data[i] + (1.0 - rho) * grad_data[i] * grad_data[i];
            
            // θ_{t+1} = θ_t - η * g_t / (√(s_t) + ε)
            param_data[i] -= learning_rate * grad_data[i] / (sqrt(s_data[i]) + epsilon);
        }
    }
    
    void print_info() const override {
        std::cout << "RMSProp Optimizer:" << std::endl;
        std::cout << "  Learning rate: " << learning_rate << std::endl;
        std::cout << "  Rho (decay): " << rho << std::endl;
        std::cout << "  Epsilon: " << epsilon << std::endl;
        std::cout << "  Tracked parameters: " << squared_grads.size() << std::endl;
    }
    
    std::unique_ptr<Optimizer> clone() const override {
        return std::make_unique<RMSProp>(learning_rate, rho, epsilon);
    }
    
    double get_learning_rate() const { return learning_rate; }
    double get_rho() const { return rho; }
    double get_epsilon() const { return epsilon; }
};

/**
 * Adam Optimizer (Adaptive Moment Estimation)
 * 
 * Mathematical Formulation:
 * m_t = β₁ * m_{t-1} + (1 - β₁) * g_t
 * v_t = β₂ * v_{t-1} + (1 - β₂) * g_t²
 * 
 * Bias correction:
 * m̂_t = m_t / (1 - β₁ᵗ)
 * v̂_t = v_t / (1 - β₂ᵗ)
 * 
 * Update:
 * θ_{t+1} = θ_t - η * m̂_t / (√(v̂_t) + ε)
 * 
 * Where:
 * - β₁: exponential decay rate for first moment (typically 0.9)
 * - β₂: exponential decay rate for second moment (typically 0.999)
 * - ε: small constant for numerical stability (typically 1e-8)
 */
class Adam : public Optimizer {
private:
    double learning_rate;
    double beta1, beta2;
    double epsilon;
    int time_step;
    
    struct AdamState {
        Tensor m; // First moment (mean)
        Tensor v; // Second moment (variance)
    };
    
    std::unordered_map<void*, AdamState> states; // Map parameter address to state
    
public:
    Adam(double learning_rate = 0.001, double beta1 = 0.9, double beta2 = 0.999, double epsilon = 1e-8)
        : learning_rate(learning_rate), beta1(beta1), beta2(beta2), epsilon(epsilon), time_step(0) {
        if (learning_rate <= 0.0) {
            throw std::invalid_argument("Learning rate must be positive");
        }
        if (beta1 < 0.0 || beta1 >= 1.0) {
            throw std::invalid_argument("Beta1 must be in [0, 1)");
        }
        if (beta2 < 0.0 || beta2 >= 1.0) {
            throw std::invalid_argument("Beta2 must be in [0, 1)");
        }
        if (epsilon <= 0.0) {
            throw std::invalid_argument("Epsilon must be positive");
        }
    }
    
    void update(Tensor& parameter, const Tensor& gradient) override {
        if (parameter.get_shape() != gradient.get_shape()) {
            throw std::invalid_argument("Parameter and gradient shapes must match");
        }
        
        // Increment global time step (shared across all parameters)
        time_step++;
        
        void* param_ptr = &parameter;
        
        // Initialize state if this is the first time seeing this parameter
        if (states.find(param_ptr) == states.end()) {
            states[param_ptr].m = Tensor(parameter.get_shape(), false);
            states[param_ptr].v = Tensor(parameter.get_shape(), false);
            states[param_ptr].m.fill(0.0);
            states[param_ptr].v.fill(0.0);
        }
        
        auto& state = states[param_ptr];
        auto& param_data = parameter.get_data();
        const auto& grad_data = gradient.get_data();
        auto& m_data = state.m.get_data();
        auto& v_data = state.v.get_data();
        
        // Bias correction factors
        double bias_correction1 = 1.0 - pow(beta1, time_step);
        double bias_correction2 = 1.0 - pow(beta2, time_step);
        
        // Update biased first and second moments, then apply bias correction and parameter update
        for (int i = 0; i < param_data.size(); ++i) {
            // Update biased first moment: m_t = β₁ * m_{t-1} + (1 - β₁) * g_t
            m_data[i] = beta1 * m_data[i] + (1.0 - beta1) * grad_data[i];
            
            // Update biased second moment: v_t = β₂ * v_{t-1} + (1 - β₂) * g_t²
            v_data[i] = beta2 * v_data[i] + (1.0 - beta2) * grad_data[i] * grad_data[i];
            
            // Compute bias-corrected first and second moments
            double m_hat = m_data[i] / bias_correction1;
            double v_hat = v_data[i] / bias_correction2;
            
            // Update parameters: θ_{t+1} = θ_t - η * m̂_t / (√(v̂_t) + ε)
            param_data[i] -= learning_rate * m_hat / (sqrt(v_hat) + epsilon);
        }
    }
    
    void print_info() const override {
        std::cout << "Adam Optimizer:" << std::endl;
        std::cout << "  Learning rate: " << learning_rate << std::endl;
        std::cout << "  Beta1: " << beta1 << std::endl;
        std::cout << "  Beta2: " << beta2 << std::endl;
        std::cout << "  Epsilon: " << epsilon << std::endl;
        std::cout << "  Time step: " << time_step << std::endl;
        std::cout << "  Tracked parameters: " << states.size() << std::endl;
    }
    
    std::unique_ptr<Optimizer> clone() const override {
        return std::make_unique<Adam>(learning_rate, beta1, beta2, epsilon);
    }
    
    void reset() {
        time_step = 0;
        states.clear();
    }
    
    double get_learning_rate() const { return learning_rate; }
    double get_beta1() const { return beta1; }
    double get_beta2() const { return beta2; }
    double get_epsilon() const { return epsilon; }
    int get_time_step() const { return time_step; }
};

/**
 * AdamW (Adam with Weight Decay)
 * 
 * Same as Adam but with decoupled weight decay instead of L2 regularization
 * 
 * Mathematical Formulation:
 * Same moment updates as Adam, but parameter update becomes:
 * θ_{t+1} = θ_t - η * (m̂_t / (√(v̂_t) + ε) + λ * θ_t)
 * 
 * Where λ is the weight decay coefficient
 */
class AdamW : public Optimizer {
private:
    double learning_rate;
    double beta1, beta2;
    double epsilon;
    double weight_decay;
    int time_step;
    
    struct AdamWState {
        Tensor m; // First moment
        Tensor v; // Second moment
    };
    
    std::unordered_map<void*, AdamWState> states;
    
public:
    AdamW(double learning_rate = 0.001, double beta1 = 0.9, double beta2 = 0.999, 
          double epsilon = 1e-8, double weight_decay = 0.01)
        : learning_rate(learning_rate), beta1(beta1), beta2(beta2), 
          epsilon(epsilon), weight_decay(weight_decay), time_step(0) {
        if (learning_rate <= 0.0) {
            throw std::invalid_argument("Learning rate must be positive");
        }
        if (weight_decay < 0.0) {
            throw std::invalid_argument("Weight decay must be non-negative");
        }
    }
    
    void update(Tensor& parameter, const Tensor& gradient) override {
        if (parameter.get_shape() != gradient.get_shape()) {
            throw std::invalid_argument("Parameter and gradient shapes must match");
        }
        
        time_step++;
        void* param_ptr = &parameter;
        
        // Initialize state if needed
        if (states.find(param_ptr) == states.end()) {
            states[param_ptr].m = Tensor(parameter.get_shape(), false);
            states[param_ptr].v = Tensor(parameter.get_shape(), false);
            states[param_ptr].m.fill(0.0);
            states[param_ptr].v.fill(0.0);
        }
        
        auto& state = states[param_ptr];
        auto& param_data = parameter.get_data();
        const auto& grad_data = gradient.get_data();
        auto& m_data = state.m.get_data();
        auto& v_data = state.v.get_data();
        
        // Bias correction factors
        double bias_correction1 = 1.0 - pow(beta1, time_step);
        double bias_correction2 = 1.0 - pow(beta2, time_step);
        
        for (int i = 0; i < param_data.size(); ++i) {
            // Update moments (same as Adam)
            m_data[i] = beta1 * m_data[i] + (1.0 - beta1) * grad_data[i];
            v_data[i] = beta2 * v_data[i] + (1.0 - beta2) * grad_data[i] * grad_data[i];
            
            // Bias-corrected moments
            double m_hat = m_data[i] / bias_correction1;
            double v_hat = v_data[i] / bias_correction2;
            
            // Update with decoupled weight decay
            param_data[i] -= learning_rate * (m_hat / (sqrt(v_hat) + epsilon) + weight_decay * param_data[i]);
        }
    }
    
    std::unique_ptr<Optimizer> clone() const override {
        return std::make_unique<AdamW>(learning_rate, beta1, beta2, epsilon, weight_decay);
    }
    
    void print_info() const override {
        std::cout << "AdamW Optimizer:" << std::endl;
        std::cout << "  Learning rate: " << learning_rate << std::endl;
        std::cout << "  Beta1: " << beta1 << std::endl;
        std::cout << "  Beta2: " << beta2 << std::endl;
        std::cout << "  Epsilon: " << epsilon << std::endl;
        std::cout << "  Weight decay: " << weight_decay << std::endl;
        std::cout << "  Time step: " << time_step << std::endl;
    }
};

/**
 * Learning Rate Scheduler
 * Adjusts learning rate during training
 */
class LearningRateScheduler {
public:
    virtual ~LearningRateScheduler() = default;
    virtual double get_lr(int epoch, double initial_lr) = 0;
};

/**
 * Step Learning Rate Scheduler
 * Reduces learning rate by factor at specified epochs
 */
class StepLR : public LearningRateScheduler {
private:
    int step_size;
    double gamma;
    
public:
    StepLR(int step_size, double gamma = 0.1) : step_size(step_size), gamma(gamma) {}
    
    double get_lr(int epoch, double initial_lr) override {
        int num_steps = epoch / step_size;
        return initial_lr * pow(gamma, num_steps);
    }
};

/**
 * Exponential Learning Rate Scheduler
 * Exponentially decays learning rate
 */
class ExponentialLR : public LearningRateScheduler {
private:
    double gamma;
    
public:
    ExponentialLR(double gamma) : gamma(gamma) {}
    
    double get_lr(int epoch, double initial_lr) override {
        return initial_lr * pow(gamma, epoch);
    }
};

/**
 * Cosine Annealing Learning Rate Scheduler
 * Uses cosine function to schedule learning rate
 */
class CosineAnnealingLR : public LearningRateScheduler {
private:
    int T_max;
    double eta_min;
    
public:
    CosineAnnealingLR(int T_max, double eta_min = 0.0) : T_max(T_max), eta_min(eta_min) {}
    
    double get_lr(int epoch, double initial_lr) override {
        return eta_min + (initial_lr - eta_min) * (1 + cos(M_PI * epoch / T_max)) / 2;
    }
};

#endif // OPTIMIZERS_HPP