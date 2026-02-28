#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <cmath>
#include <unordered_set>
#include <algorithm>

// Forward declaration
class Node;
using NodePtr = std::shared_ptr<Node>;

// Base Node class for computational graph
class Node {
public:
    std::vector<double> value;           // Node's current value (can be scalar, vector, etc.)
    std::vector<double> grad;            // Gradient w.r.t. this node
    std::vector<NodePtr> parents;        // Input nodes
    std::vector<NodePtr> children;       // Output nodes (for graph traversal)
    std::function<void()> backward_fn;   // Backward pass function
    std::string name;                    // For debugging
    bool requires_grad;                  // Whether this node needs gradients

    Node(const std::vector<double>& val, bool req_grad = true, const std::string& node_name = "")
        : value(val), requires_grad(req_grad), name(node_name) {
        grad.resize(value.size(), 0.0);
    }

    Node(double val, bool req_grad = true, const std::string& node_name = "")
        : requires_grad(req_grad), name(node_name) {
        value = {val};
        grad = {0.0};
    }

    // Forward pass - implemented by derived classes
    virtual void forward() {}

    // Backward pass - propagate gradients
    void backward() {
        if (backward_fn) {
            backward_fn();
        }
    }

    // Add child node for graph structure
    void add_child(NodePtr child) {
        children.push_back(child);
    }

    // Scalar access for convenience
    double scalar_value() const { return value.empty() ? 0.0 : value[0]; }
    double scalar_grad() const { return grad.empty() ? 0.0 : grad[0]; }

    void print_info() const {
        std::cout << "Node: " << name << " | Value: ";
        for (size_t i = 0; i < value.size(); i++) {
            std::cout << value[i];
            if (i < value.size() - 1) std::cout << ", ";
        }
        std::cout << " | Grad: ";
        for (size_t i = 0; i < grad.size(); i++) {
            std::cout << grad[i];
            if (i < grad.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
};

// Computational Graph Engine
class ComputationalGraph {
private:
    std::vector<NodePtr> tape;  // Execution tape for reverse-mode AD
    std::unordered_set<Node*> visited;  // For topological ordering

public:
    // Clear the computational graph
    void clear() {
        tape.clear();
        visited.clear();
    }

    // Add node to the tape
    void add_to_tape(NodePtr node) {
        tape.push_back(node);
    }

    // Topological sort for correct backward pass order
    void topological_sort(NodePtr node, std::vector<NodePtr>& sorted_nodes) {
        if (visited.find(node.get()) != visited.end()) {
            return;
        }
        
        visited.insert(node.get());
        
        for (auto& child : node->children) {
            topological_sort(child, sorted_nodes);
        }
        
        sorted_nodes.push_back(node);
    }

    // Reverse-mode automatic differentiation
    void backward(NodePtr loss_node) {
        if (loss_node->value.size() != 1) {
            std::cout << "Error: Loss must be a scalar!" << std::endl;
            return;
        }

        // Initialize loss gradient
        loss_node->grad[0] = 1.0;

        // Get nodes in reverse topological order
        std::vector<NodePtr> sorted_nodes;
        visited.clear();
        topological_sort(loss_node, sorted_nodes);
        
        // Reverse the order for backward pass
        std::reverse(sorted_nodes.begin(), sorted_nodes.end());

        std::cout << "\n=== Backward Pass ===" << std::endl;
        
        // Propagate gradients in reverse order
        for (auto& node : sorted_nodes) {
            if (node->requires_grad) {
                std::cout << "Processing: " << node->name << std::endl;
                node->backward();
                node->print_info();
            }
        }
    }

    // Display the entire computation graph
    void display_graph() {
        std::cout << "\n=== Computational Graph ===" << std::endl;
        for (auto& node : tape) {
            node->print_info();
        }
    }
};

// Global computational graph instance
ComputationalGraph comp_graph;

// Factory functions for creating computational nodes

// Variable node (leaf node with requires_grad=True)
NodePtr Variable(double value, const std::string& name = "var") {
    auto node = std::make_shared<Node>(value, true, name);
    comp_graph.add_to_tape(node);
    return node;
}

NodePtr Variable(const std::vector<double>& value, const std::string& name = "var") {
    auto node = std::make_shared<Node>(value, true, name);
    comp_graph.add_to_tape(node);
    return node;
}

// Constant node (no gradients needed)
NodePtr Constant(double value, const std::string& name = "const") {
    auto node = std::make_shared<Node>(value, false, name);
    comp_graph.add_to_tape(node);
    return node;
}

// Addition operation
NodePtr Add(NodePtr a, NodePtr b, const std::string& name = "add") {
    auto result = std::make_shared<Node>(a->scalar_value() + b->scalar_value(), true, name);
    result->parents = {a, b};
    
    // Add this node as child to parents
    a->add_child(result);
    b->add_child(result);
    
    // Define backward pass
    result->backward_fn = [a, b, result]() {
        if (a->requires_grad) {
            a->grad[0] += result->grad[0];  // ∂(a+b)/∂a = 1
        }
        if (b->requires_grad) {
            b->grad[0] += result->grad[0];  // ∂(a+b)/∂b = 1
        }
    };
    
    comp_graph.add_to_tape(result);
    return result;
}

// Multiplication operation
NodePtr Multiply(NodePtr a, NodePtr b, const std::string& name = "mul") {
    auto result = std::make_shared<Node>(a->scalar_value() * b->scalar_value(), true, name);
    result->parents = {a, b};
    
    a->add_child(result);
    b->add_child(result);
    
    result->backward_fn = [a, b, result]() {
        if (a->requires_grad) {
            a->grad[0] += b->scalar_value() * result->grad[0];  // ∂(a*b)/∂a = b
        }
        if (b->requires_grad) {
            b->grad[0] += a->scalar_value() * result->grad[0];  // ∂(a*b)/∂b = a
        }
    };
    
    comp_graph.add_to_tape(result);
    return result;
}

// Power operation
NodePtr Power(NodePtr base, double exponent, const std::string& name = "pow") {
    double base_val = base->scalar_value();
    auto result = std::make_shared<Node>(std::pow(base_val, exponent), true, name);
    result->parents = {base};
    
    base->add_child(result);
    
    result->backward_fn = [base, exponent, result]() {
        if (base->requires_grad) {
            double base_val = base->scalar_value();
            // ∂(base^exp)/∂base = exp * base^(exp-1)
            base->grad[0] += exponent * std::pow(base_val, exponent - 1) * result->grad[0];
        }
    };
    
    comp_graph.add_to_tape(result);
    return result;
}

// Exponential operation
NodePtr Exp(NodePtr x, const std::string& name = "exp") {
    double x_val = x->scalar_value();
    auto result = std::make_shared<Node>(std::exp(x_val), true, name);
    result->parents = {x};
    
    x->add_child(result);
    
    result->backward_fn = [x, result]() {
        if (x->requires_grad) {
            // ∂(e^x)/∂x = e^x
            x->grad[0] += result->scalar_value() * result->grad[0];
        }
    };
    
    comp_graph.add_to_tape(result);
    return result;
}

// Logarithm operation
NodePtr Log(NodePtr x, const std::string& name = "log") {
    double x_val = x->scalar_value();
    auto result = std::make_shared<Node>(std::log(x_val), true, name);
    result->parents = {x};
    
    x->add_child(result);
    
    result->backward_fn = [x, result]() {
        if (x->requires_grad) {
            // ∂(ln(x))/∂x = 1/x
            x->grad[0] += (1.0 / x->scalar_value()) * result->grad[0];
        }
    };
    
    comp_graph.add_to_tape(result);
    return result;
}

// Sine operation
NodePtr Sin(NodePtr x, const std::string& name = "sin") {
    double x_val = x->scalar_value();
    auto result = std::make_shared<Node>(std::sin(x_val), true, name);
    result->parents = {x};
    
    x->add_child(result);
    
    result->backward_fn = [x, result]() {
        if (x->requires_grad) {
            // ∂(sin(x))/∂x = cos(x)
            x->grad[0] += std::cos(x->scalar_value()) * result->grad[0];
        }
    };
    
    comp_graph.add_to_tape(result);
    return result;
}

// Sigmoid activation function (important for neural networks)
NodePtr Sigmoid(NodePtr x, const std::string& name = "sigmoid") {
    double x_val = x->scalar_value();
    double sigmoid_val = 1.0 / (1.0 + std::exp(-x_val));
    auto result = std::make_shared<Node>(sigmoid_val, true, name);
    result->parents = {x};
    
    x->add_child(result);
    
    result->backward_fn = [x, result]() {
        if (x->requires_grad) {
            double sig_val = result->scalar_value();
            // ∂(sigmoid(x))/∂x = sigmoid(x) * (1 - sigmoid(x))
            x->grad[0] += sig_val * (1.0 - sig_val) * result->grad[0];
        }
    };
    
    comp_graph.add_to_tape(result);
    return result;
}

// ReLU activation function
NodePtr ReLU(NodePtr x, const std::string& name = "relu") {
    double x_val = x->scalar_value();
    auto result = std::make_shared<Node>(std::max(0.0, x_val), true, name);
    result->parents = {x};
    
    x->add_child(result);
    
    result->backward_fn = [x, result]() {
        if (x->requires_grad) {
            // ∂(ReLU(x))/∂x = 1 if x > 0, else 0
            double derivative = (x->scalar_value() > 0.0) ? 1.0 : 0.0;
            x->grad[0] += derivative * result->grad[0];
        }
    };
    
    comp_graph.add_to_tape(result);
    return result;
}

// Mean Squared Error Loss
NodePtr MSELoss(NodePtr predicted, NodePtr target, const std::string& name = "mse_loss") {
    double diff = predicted->scalar_value() - target->scalar_value();
    auto result = std::make_shared<Node>(0.5 * diff * diff, true, name);
    result->parents = {predicted, target};
    
    predicted->add_child(result);
    target->add_child(result);
    
    result->backward_fn = [predicted, target, result]() {
        double diff = predicted->scalar_value() - target->scalar_value();
        if (predicted->requires_grad) {
            // ∂(MSE)/∂predicted = (predicted - target)
            predicted->grad[0] += diff * result->grad[0];
        }
        if (target->requires_grad) {
            // ∂(MSE)/∂target = -(predicted - target)
            target->grad[0] += -diff * result->grad[0];
        }
    };
    
    comp_graph.add_to_tape(result);
    return result;
}

// Demonstration functions
class AutogradDemo {
public:
    static void simple_operations_demo() {
        std::cout << "\n=== Simple Operations Demo ===" << std::endl;
        comp_graph.clear();
        
        // Create variables
        auto x = Variable(3.0, "x");
        auto y = Variable(2.0, "y");
        
        // Build computation: z = x^2 + 2*x*y + y^2
        auto x_squared = Power(x, 2.0, "x^2");
        auto xy = Multiply(x, y, "x*y");
        auto two_xy = Multiply(Constant(2.0, "2"), xy, "2*x*y");
        auto y_squared = Power(y, 2.0, "y^2");
        auto temp = Add(x_squared, two_xy, "x^2+2xy");
        auto z = Add(temp, y_squared, "z=x^2+2xy+y^2");
        
        std::cout << "Forward pass complete. z = " << z->scalar_value() << std::endl;
        
        // Backward pass
        comp_graph.backward(z);
        
        std::cout << "\nFinal gradients:" << std::endl;
        std::cout << "∂z/∂x = " << x->scalar_grad() << " (expected: 2x+2y = " << 2*3 + 2*2 << ")" << std::endl;
        std::cout << "∂z/∂y = " << y->scalar_grad() << " (expected: 2x+2y = " << 2*3 + 2*2 << ")" << std::endl;
    }
    
    static void neural_network_demo() {
        std::cout << "\n=== Neural Network Demo ===" << std::endl;
        comp_graph.clear();
        
        // Simple neural network: input -> linear -> sigmoid -> output
        auto input = Variable(1.5, "input");
        auto weight = Variable(0.5, "weight");
        auto bias = Variable(0.1, "bias");
        auto target = Constant(0.8, "target");
        
        // Forward pass: linear transformation
        auto linear = Add(Multiply(input, weight, "input*weight"), bias, "linear");
        
        // Activation function
        auto output = Sigmoid(linear, "output");
        
        // Loss function
        auto loss = MSELoss(output, target, "loss");
        
        std::cout << "Neural Network Forward Pass:" << std::endl;
        std::cout << "Input: " << input->scalar_value() << std::endl;
        std::cout << "Linear: " << linear->scalar_value() << std::endl;
        std::cout << "Output (sigmoid): " << output->scalar_value() << std::endl;
        std::cout << "Target: " << target->scalar_value() << std::endl;
        std::cout << "Loss: " << loss->scalar_value() << std::endl;
        
        // Backward pass
        comp_graph.backward(loss);
        
        std::cout << "\nGradients for backpropagation:" << std::endl;
        std::cout << "∂L/∂weight = " << weight->scalar_grad() << std::endl;
        std::cout << "∂L/∂bias = " << bias->scalar_grad() << std::endl;
        std::cout << "∂L/∂input = " << input->scalar_grad() << std::endl;
    }
    
    static void chain_rule_demo() {
        std::cout << "\n=== Chain Rule Demo ===" << std::endl;
        comp_graph.clear();
        
        // Demonstrate chain rule: f(g(h(x))) where h(x)=x^2, g(u)=sin(u), f(v)=exp(v)
        auto x = Variable(1.0, "x");
        
        auto h = Power(x, 2.0, "h=x^2");      // h(x) = x^2
        auto g = Sin(h, "g=sin(h)");          // g(h) = sin(h) = sin(x^2)
        auto f = Exp(g, "f=exp(g)");          // f(g) = exp(g) = exp(sin(x^2))
        
        std::cout << "Chain: f(g(h(x))) = exp(sin(x^2))" << std::endl;
        std::cout << "x = " << x->scalar_value() << std::endl;
        std::cout << "h(x) = x^2 = " << h->scalar_value() << std::endl;
        std::cout << "g(h) = sin(h) = " << g->scalar_value() << std::endl;
        std::cout << "f(g) = exp(g) = " << f->scalar_value() << std::endl;
        
        // Backward pass
        comp_graph.backward(f);
        
        // Manual calculation of derivative using chain rule:
        // df/dx = df/dg * dg/dh * dh/dx
        // df/dg = exp(g)
        // dg/dh = cos(h)
        // dh/dx = 2x
        double manual_grad = std::exp(g->scalar_value()) * std::cos(h->scalar_value()) * 2 * x->scalar_value();
        
        std::cout << "\nChain rule gradient:" << std::endl;
        std::cout << "Computed ∂f/∂x = " << x->scalar_grad() << std::endl;
        std::cout << "Manual ∂f/∂x = " << manual_grad << std::endl;
        std::cout << "Difference: " << std::abs(x->scalar_grad() - manual_grad) << std::endl;
    }
};