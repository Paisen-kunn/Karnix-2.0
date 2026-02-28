#ifndef TENSOR_HPP
#define TENSOR_HPP

#include <vector>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <cmath>

/**
 * Tensor class for deep learning operations
 * Supports channels-first format: [C, H, W] or [N, C, H, W] for batches
 * Provides automatic gradient tracking and memory management
 */
class Tensor {
private:
    std::vector<double> data;
    std::vector<double> grad;
    std::vector<int> shape;
    std::vector<int> strides;
    bool requires_grad;
    
    void compute_strides() {
        strides.resize(shape.size());
        if (!shape.empty()) {
            strides.back() = 1;
            for (int i = shape.size() - 2; i >= 0; --i) {
                strides[i] = strides[i + 1] * shape[i + 1];
            }
        }
    }

public:
    // Constructors
    Tensor() : requires_grad(false) {}
    
    Tensor(const std::vector<int>& shape, bool requires_grad = false) 
        : shape(shape), requires_grad(requires_grad) {
        compute_strides();
        int total_size = get_total_size();
        data.resize(total_size, 0.0);
        if (requires_grad) {
            grad.resize(total_size, 0.0);
        }
    }
    
    Tensor(const std::vector<int>& shape, const std::vector<double>& values, bool requires_grad = false)
        : shape(shape), requires_grad(requires_grad) {
        compute_strides();
        int total_size = get_total_size();
        if (values.size() != total_size) {
            throw std::invalid_argument("Data size doesn't match tensor shape");
        }
        data = values;
        if (requires_grad) {
            grad.resize(total_size, 0.0);
        }
    }
    
    // Shape and size methods
    const std::vector<int>& get_shape() const { return shape; }
    const std::vector<int>& get_strides() const { return strides; }
    int get_total_size() const {
        return std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());
    }
    int get_dims() const { return shape.size(); }
    
    // Data access
    std::vector<double>& get_data() { return data; }
    const std::vector<double>& get_data() const { return data; }
    std::vector<double>& get_grad() { return grad; }
    const std::vector<double>& get_grad() const { return grad; }
    
    bool get_requires_grad() const { return requires_grad; }
    void set_requires_grad(bool req_grad) { 
        requires_grad = req_grad;
        if (requires_grad && grad.empty()) {
            grad.resize(data.size(), 0.0);
        }
    }
    
    // Index computation
    int compute_flat_index(const std::vector<int>& indices) const {
        if (indices.size() != shape.size()) {
            throw std::invalid_argument("Index dimensions don't match tensor shape");
        }
        int flat_idx = 0;
        for (int i = 0; i < indices.size(); ++i) {
            if (indices[i] < 0 || indices[i] >= shape[i]) {
                throw std::out_of_range("Index out of bounds");
            }
            flat_idx += indices[i] * strides[i];
        }
        return flat_idx;
    }
    
    // Element access
    double& operator()(const std::vector<int>& indices) {
        return data[compute_flat_index(indices)];
    }
    
    const double& operator()(const std::vector<int>& indices) const {
        return data[compute_flat_index(indices)];
    }
    
    // 1D access
    double& operator[](int idx) {
        if (idx < 0 || idx >= data.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        return data[idx];
    }
    
    const double& operator[](int idx) const {
        if (idx < 0 || idx >= data.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        return data[idx];
    }
    
    // Convenient access for common shapes
    // 2D tensor [M, N] (matrices)
    double& operator()(int m, int n) {
        if (shape.size() != 2) {
            throw std::invalid_argument("2D accessor requires 2D tensor");
        }
        return data[m * strides[0] + n * strides[1]];
    }
    
    const double& operator()(int m, int n) const {
        if (shape.size() != 2) {
            throw std::invalid_argument("2D accessor requires 2D tensor");
        }
        return data[m * strides[0] + n * strides[1]];
    }
    
    // 3D tensor [C, H, W]
    double& operator()(int c, int h, int w) {
        if (shape.size() != 3) {
            throw std::invalid_argument("3D accessor requires 3D tensor");
        }
        return data[c * strides[0] + h * strides[1] + w * strides[2]];
    }
    
    const double& operator()(int c, int h, int w) const {
        if (shape.size() != 3) {
            throw std::invalid_argument("3D accessor requires 3D tensor");
        }
        return data[c * strides[0] + h * strides[1] + w * strides[2]];
    }
    
    // 4D tensor [N, C, H, W]
    double& operator()(int n, int c, int h, int w) {
        if (shape.size() != 4) {
            throw std::invalid_argument("4D accessor requires 4D tensor");
        }
        return data[n * strides[0] + c * strides[1] + h * strides[2] + w * strides[3]];
    }
    
    const double& operator()(int n, int c, int h, int w) const {
        if (shape.size() != 4) {
            throw std::invalid_argument("4D accessor requires 4D tensor");
        }
        return data[n * strides[0] + c * strides[1] + h * strides[2] + w * strides[3]];
    }
    
    // Gradient access
    double& grad_at(const std::vector<int>& indices) {
        if (!requires_grad) {
            throw std::runtime_error("Tensor doesn't require gradients");
        }
        return grad[compute_flat_index(indices)];
    }
    
    const double& grad_at(const std::vector<int>& indices) const {
        if (!requires_grad) {
            throw std::runtime_error("Tensor doesn't require gradients");
        }
        return grad[compute_flat_index(indices)];
    }
    
    // Operations
    void zero_grad() {
        if (requires_grad) {
            std::fill(grad.begin(), grad.end(), 0.0);
        }
    }
    
    void fill(double value) {
        std::fill(data.begin(), data.end(), value);
    }
    
    void random_normal(double mean = 0.0, double std = 1.0) {
        // Simple Box-Muller transform for normal distribution
        for (int i = 0; i < data.size(); i += 2) {
            double u1 = (rand() + 1.0) / (RAND_MAX + 1.0);
            double u2 = (rand() + 1.0) / (RAND_MAX + 1.0);
            double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
            double z1 = sqrt(-2.0 * log(u1)) * sin(2.0 * M_PI * u2);
            
            data[i] = mean + std * z0;
            if (i + 1 < data.size()) {
                data[i + 1] = mean + std * z1;
            }
        }
    }
    
    void xavier_init() {
        if (shape.size() >= 2) {
            int fan_in = shape[shape.size() - 1];  // Input features
            int fan_out = shape[shape.size() - 2]; // Output features
            double limit = sqrt(6.0 / (fan_in + fan_out));
            
            for (auto& val : data) {
                val = ((double)rand() / RAND_MAX) * 2.0 * limit - limit;
            }
        }
    }
    
    // Reshape (must preserve total size)
    void reshape(const std::vector<int>& new_shape) {
        int new_total = std::accumulate(new_shape.begin(), new_shape.end(), 1, std::multiplies<int>());
        if (new_total != get_total_size()) {
            throw std::invalid_argument("New shape must have same total size");
        }
        shape = new_shape;
        compute_strides();
    }
    
    // Clone tensor
    Tensor clone() const {
        Tensor result(shape, data, requires_grad);
        if (requires_grad) {
            result.grad = grad;
        }
        return result;
    }
    
    // Math operations
    void add_(const Tensor& other) {
        if (shape != other.shape) {
            throw std::invalid_argument("Tensor shapes must match for addition");
        }
        for (int i = 0; i < data.size(); ++i) {
            data[i] += other.data[i];
        }
    }
    
    void sub_(const Tensor& other) {
        if (shape != other.shape) {
            throw std::invalid_argument("Tensor shapes must match for subtraction");
        }
        for (int i = 0; i < data.size(); ++i) {
            data[i] -= other.data[i];
        }
    }
    
    void mul_(double scalar) {
        for (auto& val : data) {
            val *= scalar;
        }
    }
    
    // Statistics
    double sum() const {
        return std::accumulate(data.begin(), data.end(), 0.0);
    }
    
    double mean() const {
        return sum() / data.size();
    }
    
    double max() const {
        return *std::max_element(data.begin(), data.end());
    }
    
    double min() const {
        return *std::min_element(data.begin(), data.end());
    }
    
    // Display
    void print(const std::string& name = "") const {
        if (!name.empty()) {
            std::cout << name << " ";
        }
        std::cout << "Tensor(shape=[";
        for (int i = 0; i < shape.size(); ++i) {
            std::cout << shape[i];
            if (i < shape.size() - 1) std::cout << ", ";
        }
        std::cout << "]):" << std::endl;
        
        // For small tensors, print all values
        if (data.size() <= 50) {
            print_recursive(0, {});
        } else {
            std::cout << "  [" << data.size() << " elements] ";
            std::cout << "min=" << std::fixed << std::setprecision(4) << min() 
                     << ", max=" << max() << ", mean=" << mean() << std::endl;
        }
    }
    
private:
    void print_recursive(int dim, std::vector<int> indices) const {
        if (dim == shape.size()) {
            std::cout << std::fixed << std::setprecision(4) << data[compute_flat_index(indices)];
            return;
        }
        
        std::string indent(dim * 2, ' ');
        std::cout << indent << "[";
        
        for (int i = 0; i < shape[dim]; ++i) {
            indices.push_back(i);
            
            if (dim == shape.size() - 1) {
                if (i > 0) std::cout << ", ";
                print_recursive(dim + 1, indices);
            } else {
                if (i > 0) std::cout << ",";
                std::cout << std::endl;
                print_recursive(dim + 1, indices);
            }
            
            indices.pop_back();
        }
        
        std::cout << "]";
        if (dim == 0) std::cout << std::endl;
    }
};

// Utility functions for tensor operations
namespace TensorUtils {
    // Create tensors with specific initialization
    inline Tensor zeros(const std::vector<int>& shape, bool requires_grad = false) {
        return Tensor(shape, requires_grad);
    }
    
    inline Tensor ones(const std::vector<int>& shape, bool requires_grad = false) {
        Tensor result(shape, requires_grad);
        result.fill(1.0);
        return result;
    }
    
    inline Tensor randn(const std::vector<int>& shape, bool requires_grad = false) {
        Tensor result(shape, requires_grad);
        result.random_normal();
        return result;
    }
    
    // Compute output size for convolution
    inline std::vector<int> conv_output_size(const std::vector<int>& input_shape,
                                           const std::vector<int>& kernel_shape,
                                           const std::vector<int>& stride,
                                           const std::vector<int>& padding) {
        // input_shape: [C_in, H_in, W_in] or [N, C_in, H_in, W_in]
        // kernel_shape: [C_out, C_in, kH, kW]
        // returns: [C_out, H_out, W_out] or [N, C_out, H_out, W_out]
        
        std::vector<int> output_shape;
        
        if (input_shape.size() == 4) {
            // Batch dimension
            output_shape.push_back(input_shape[0]); // N
            output_shape.push_back(kernel_shape[0]); // C_out
            
            int h_out = (input_shape[2] + 2 * padding[0] - kernel_shape[2]) / stride[0] + 1;
            int w_out = (input_shape[3] + 2 * padding[1] - kernel_shape[3]) / stride[1] + 1;
            
            output_shape.push_back(h_out);
            output_shape.push_back(w_out);
        } else if (input_shape.size() == 3) {
            // No batch dimension
            output_shape.push_back(kernel_shape[0]); // C_out
            
            int h_out = (input_shape[1] + 2 * padding[0] - kernel_shape[2]) / stride[0] + 1;
            int w_out = (input_shape[2] + 2 * padding[1] - kernel_shape[3]) / stride[1] + 1;
            
            output_shape.push_back(h_out);
            output_shape.push_back(w_out);
        }
        
        return output_shape;
    }
    
    // Compute output size for pooling
    inline std::vector<int> pool_output_size(const std::vector<int>& input_shape,
                                           int pool_size,
                                           int stride) {
        std::vector<int> output_shape = input_shape;
        
        if (input_shape.size() == 4) {
            // [N, C, H, W]
            output_shape[2] = (input_shape[2] - pool_size) / stride + 1;
            output_shape[3] = (input_shape[3] - pool_size) / stride + 1;
        } else if (input_shape.size() == 3) {
            // [C, H, W]
            output_shape[1] = (input_shape[1] - pool_size) / stride + 1;
            output_shape[2] = (input_shape[2] - pool_size) / stride + 1;
        }
        
        return output_shape;
    }
}

#endif // TENSOR_HPP