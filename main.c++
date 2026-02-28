#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <string>
#include <algorithm>
#include <random>
#include <cassert>
#include <numeric>
#include <functional>
#include <cstdlib>
#include <ctime>

#include "mathematical/Vector/VectorOperations.hpp"
#include "mathematical/matrix/MatrixOperations.hpp"
#include "mathematical/matrix/AdvancedMatrix.hpp"
#include "mathematical/Calc/Calc.hpp"
#include "mathematical/Calc/CalcEng.hpp"
#include "mathematical/Graph/GraphVisualizer.hpp"
#include "mathematical/ComputationalGraph/ComputationalGraph.hpp"

using namespace std;

int main(){
    srand(static_cast<unsigned>(time(nullptr)));
    
    cout << string(80, '=') << endl;
    cout << "KARNIX - COMPLETE NEURAL NETWORK & MATHEMATICAL COMPUTING SYSTEM" << endl;
    cout << string(80, '=') << endl;
    
    cout << "\n=== MATHEMATICAL FOUNDATIONS ===" << endl;
    
    cout << "\n--- Vector Operations ---" << endl;
    VectorOperations vo;
    int arr1[] = {3, 4, 5};
    int arr2[] = {1, 2, 3};
    vo.setVectors(arr1, 3, arr2, 3);
    
    cout << "Vector 1: ";
    vo.displayVector1();
    cout << "Vector 2: ";
    vo.displayVector2();
    
    vo.vector_add();
    cout << "Addition Result: ";
    vo.displayAddResult();
    
    vo.vector_sub();
    cout << "Subtraction Result: ";
    vo.displaySubResult();
    
    vo.scalarVectorMultiply(2);
    cout << "Scalar Multiplication Result: ";
    vo.displayScalarResult();
    
    vo.vector_norm();
    cout << "Vector Norm (Magnitude): ";
    vo.displayNormResult();

    cout << "\n--- Matrix Operations ---" << endl;
    MatrixOperations mo;
    
    vector<vector<int>> matrix1 = {
        {1, 2, 3},
        {4, 5, 6}
    };
    
    vector<vector<int>> matrix2 = {
        {7, 8, 9},
        {10, 11, 12}
    };
    
    mo.setMatrices(matrix1, matrix2);
    mo.displayMatrix1();
    mo.displayMatrix2();
    
    mo.matrix_add();
    mo.displayAddResult();
    
    mo.matrix_sub();
    mo.displaySubResult();
    
    mo.setMatrix(matrix1);
    mo.scalar_multiply(3);
    mo.displayScalarResult();
    
    mo.matrix_transpose();
    mo.displayTransposeResult();
    
    mo.displayNorm();

    cout << "\n=== Advanced Matrix Operations ===" << endl;
    AdvancedMatrix am;
    
    // Create a 2x2 matrix for eigenvalue demonstration
    vector<vector<double>> square_matrix = {
        {4.0, 2.0},
        {1.0, 3.0}
    };
    
    am.setMatrix(square_matrix);
    am.displayMatrix();
    
    // Matrix trace
    am.displayTrace();
    
    // Matrix rank
    am.displayRank();
    
    // Eigenvalues (conceptual)
    am.displayEigenvalues();
    
    // Eigenvectors (conceptual)
    am.displayEigenvectors();
    
    // Matrix projection
    vector<double> vector_b = {5.0, 3.0};
    am.displayProjection(vector_b);
    
    // Test with a 3x3 matrix
    cout << "\n=== 3x3 Matrix Example ===" << endl;
    vector<vector<double>> matrix_3x3 = {
        {1.0, 2.0, 3.0},
        {4.0, 5.0, 6.0},
        {7.0, 8.0, 9.0}
    };
    
    am.setMatrix(matrix_3x3);
    am.displayMatrix();
    am.displayTrace();
    am.displayRank();
    am.displayEigenvalues(); // Will show conceptual message
    
    vector<double> vector_b2 = {1.0, 2.0, 3.0};
    am.displayProjection(vector_b2);

    cout << "\n=== Calculus Operations ===" << endl;
    Calc calc;
    double x = 2.0;
    double y = 3.0;
    
    // Constant function
    cout << "Constant function f(x) = 5: " << calc.constant(x, 5.0) << endl;
    cout << "Derivative of constant: " << calc.constant_derivative(x) << endl;
    
    // Linear function
    cout << "Linear function f(x) = 2x + 3: " << calc.linear(x, 2.0, 3.0) << endl;
    
    // Polynomial function
    vector<double> coeffs = {1.0, 2.0, 3.0}; // 1 + 2x + 3x²
    cout << "Polynomial f(x) = 1 + 2x + 3x² at x=" << x << ": " << calc.polynomial(x, coeffs) << endl;
    cout << "Polynomial derivative at x=" << x << ": " << calc.polynomial_derivative(x, coeffs) << endl;
    
    // Exponential function
    cout << "Exponential e^x at x=" << x << ": " << calc.exponential(x) << endl;
    cout << "Exponential derivative at x=" << x << ": " << calc.exponential_derivative(x) << endl;
    
    // Logarithmic function
    cout << "Natural log ln(x) at x=" << x << ": " << calc.logarithmic(x) << endl;
    cout << "Logarithmic derivative at x=" << x << ": " << calc.logarithmic_derivative(x) << endl;
    
    // Trigonometric functions
    cout << "Sine sin(x) at x=" << x << ": " << calc.sine(x) << endl;
    cout << "Sine derivative cos(x) at x=" << x << ": " << calc.sine_derivative(x) << endl;
    cout << "Cosine cos(x) at x=" << x << ": " << calc.cosine(x) << endl;
    cout << "Cosine derivative -sin(x) at x=" << x << ": " << calc.cosine_derivative(x) << endl;
    
    // Composite function
    cout << "Composite function f(g(x)) where g(x)=x+1, f(u)=u² at x=" << x << ": " << calc.f_of_g(x) << endl;
    cout << "Composite derivative using chain rule at x=" << x << ": " << calc.composite_derivative(x) << endl;
    
    // Multivariate function
    cout << "Multivariate f(x,y) = x² + y² at (" << x << "," << y << "): " << calc.multivariate(x, y) << endl;
    auto gradient = calc.multivariate_gradient(x, y);
    cout << "Gradient at (" << x << "," << y << "): (" << gradient.first << ", " << gradient.second << ")" << endl;

    cout << "\n=== AI/ML Advanced Calculus ===" << endl;
    CalcEng calcEng;
    
    // Demonstrate the 5 AI calculus concepts
    calcEng.demonstrateAICalculus();
    
    // Individual demonstrations
    cout << "\n--- Individual AI Calculus Operations ---" << endl;
    
    // 1. Differentiation for neural network backpropagation
    auto relu = [&calcEng](double x) { return calcEng.neural_activation_relu(x); };
    calcEng.differentiation(relu, 2.0);
    calcEng.displayDifferentiation(2.0);
    
    // 2. Integration for probability calculations
    auto probability_density = [](double x) { return std::exp(-x*x/2) / std::sqrt(2*M_PI); };
    calcEng.integration(probability_density, -1.0, 1.0);
    calcEng.displayIntegration(-1.0, 1.0);
    
    // 3. Gradient for loss function optimization
    auto loss_func = [](const std::vector<double>& w) {
        return w[0]*w[0] + w[1]*w[1] + 2*w[0]*w[1]; // Simple quadratic loss
    };
    std::vector<double> weights = {1.5, 2.5};
    calcEng.gradient(loss_func, weights);
    calcEng.displayGradient(weights);
    
    // 4. Jacobian for neural network layer transformations
    std::vector<std::function<double(std::vector<double>)>> network_funcs = {
        [](const std::vector<double>& x) { return x[0] + x[1]; },
        [](const std::vector<double>& x) { return x[0] * x[1]; }
    };
    std::vector<double> inputs = {1.0, 2.0};
    calcEng.jacobian(network_funcs, inputs);
    calcEng.displayJacobian(inputs);
    
    // 5. Hessian for second-order optimization (Newton's method)
    auto optimization_func = [](const std::vector<double>& x) {
        return x[0]*x[0] + x[1]*x[1] - 2*x[0] - 4*x[1] + 5; // Convex quadratic
    };
    std::vector<double> opt_point = {0.5, 1.0};
    calcEng.hessian(optimization_func, opt_point);
    calcEng.displayHessian(opt_point);

    cout << "\n=== Graph Visualizations ===" << endl;
    GraphVisualizer graph;
    
    // 1. Sine Wave
    cout << "\n1. Trigonometric Function:" << endl;
    graph.plotSineWave();
    
    // 2. Quadratic Function
    cout << "\n2. Quadratic Function:" << endl;
    graph.plotQuadratic();
    
    // 3. Exponential Function
    cout << "\n3. Exponential Growth:" << endl;
    graph.plotExponential();
    
    // 4. Logarithmic Function
    cout << "\n4. Logarithmic Function:" << endl;
    graph.plotLogarithmic();
    
    // 5. Function and its Derivative
    cout << "\n5. Function and Derivative Comparison:" << endl;
    graph.plotDerivative();
    
    // 6. Gradient Field Visualization
    cout << "\n6. Gradient Field (AI Optimization Context):" << endl;
    graph.plotGradientField();
    
    // 7. Contour Plot
    cout << "\n7. Contour Plot (Loss Function Landscape):" << endl;
    graph.plotContour();
    
    // 8. Data Scatter Plot with Trend Line
    cout << "\n8. Data Analysis - Scatter Plot with Regression:" << endl;
    graph.plotDataScatter();
    
    // 9. Custom AI/ML Function Visualization
    cout << "\n9. Custom AI Function Visualization:" << endl;
    graph.setTitle("Neural Network Activation: Sigmoid vs ReLU");
    graph.setRange(-5, 5, -0.5, 5);
    graph.clearCanvas();
    graph.drawAxes();
    
    // Plot sigmoid activation function
    graph.plotFunction([](double x) { return 1.0 / (1.0 + std::exp(-x)); }, 's');
    
    // Plot ReLU activation function
    graph.plotFunction([](double x) { return std::max(0.0, x); }, 'r');
    
    graph.addScale();
    graph.display();
    cout << "Legend: s = Sigmoid, r = ReLU" << endl;
    
    // 10. Loss Function Visualization
    cout << "\n10. Loss Function Landscape:" << endl;
    graph.setTitle("Quadratic Loss Function: L = (x-2)² + (y-1)²");
    graph.setRange(-1, 5, -2, 4);
    graph.clearCanvas();
    graph.drawAxes();
    
    // Plot minimum point
    std::vector<std::pair<double, double>> minimum = {{2.0, 1.0}};
    graph.plotPoints(minimum, 'M');
    
    // Plot some gradient descent path
    std::vector<std::pair<double, double>> descent_path = {
        {4.5, 3.5}, {4.0, 3.0}, {3.5, 2.5}, {3.0, 2.0}, {2.5, 1.5}, {2.0, 1.0}
    };
    graph.plotPoints(descent_path, '*');
    
    graph.addScale();
    graph.display();
    cout << "M = Global Minimum, * = Gradient Descent Path" << endl;

    cout << "\n=== Computational Graph & Autograd ===" << endl;
    
    // 1. Simple operations demonstration
    AutogradDemo::simple_operations_demo();
    
    // 2. Neural network backpropagation
    AutogradDemo::neural_network_demo();
    
    // 3. Chain rule demonstration
    AutogradDemo::chain_rule_demo();
    
    // 4. Advanced autograd example - polynomial regression
    cout << "\n=== Polynomial Regression with Autograd ===" << endl;
    comp_graph.clear();
    
    // Data point: y = 2x^3 - 3x^2 + x + 1 at x = 2
    auto x_data = Constant(2.0, "x_data");
    auto y_target = Constant(2*8 - 3*4 + 2 + 1, "y_target"); // = 11
    
    // Model parameters (to be learned)
    auto a = Variable(1.0, "a");  // coefficient for x^3
    auto b = Variable(1.0, "b");  // coefficient for x^2  
    auto c = Variable(1.0, "c");  // coefficient for x
    auto d = Variable(1.0, "d");  // constant term
    
    // Model: y = a*x^3 + b*x^2 + c*x + d
    auto x_cubed = Power(x_data, 3.0, "x^3");
    auto x_squared = Power(x_data, 2.0, "x^2");
    
    auto term1 = Multiply(a, x_cubed, "a*x^3");
    auto term2 = Multiply(b, x_squared, "b*x^2");
    auto term3 = Multiply(c, x_data, "c*x");
    
    auto sum1 = Add(term1, term2, "a*x^3+b*x^2");
    auto sum2 = Add(sum1, term3, "...+c*x");
    auto y_pred = Add(sum2, d, "y_predicted");
    
    // Loss function
    auto loss = MSELoss(y_pred, y_target, "polynomial_loss");
    
    cout << "Polynomial Model: y = a*x^3 + b*x^2 + c*x + d" << endl;
    cout << "Data point: x = " << x_data->scalar_value() << ", y_target = " << y_target->scalar_value() << endl;
    cout << "Initial prediction: " << y_pred->scalar_value() << endl;
    cout << "Initial loss: " << loss->scalar_value() << endl;
    
    // Backward pass
    comp_graph.backward(loss);
    
    cout << "\nGradients for parameter updates:" << endl;
    cout << "∂L/∂a = " << a->scalar_grad() << " (should move a towards 2)" << endl;
    cout << "∂L/∂b = " << b->scalar_grad() << " (should move b towards -3)" << endl;
    cout << "∂L/∂c = " << c->scalar_grad() << " (should move c towards 1)" << endl;
    cout << "∂L/∂d = " << d->scalar_grad() << " (should move d towards 1)" << endl;
    
    // 5. Activation function comparison
    cout << "\n=== Activation Function Gradients ===" << endl;
    comp_graph.clear();
    
    auto x_act = Variable(0.5, "x");
    auto sigmoid_out = Sigmoid(x_act, "sigmoid");
    auto relu_out = ReLU(x_act, "relu");
    
    cout << "Input: " << x_act->scalar_value() << endl;
    cout << "Sigmoid output: " << sigmoid_out->scalar_value() << endl;
    cout << "ReLU output: " << relu_out->scalar_value() << endl;
    
    // Test sigmoid gradient
    comp_graph.backward(sigmoid_out);
    double sigmoid_grad = x_act->scalar_grad();
    
    // Reset gradients
    x_act->grad[0] = 0.0;
    
    // Test ReLU gradient  
    comp_graph.backward(relu_out);
    double relu_grad = x_act->scalar_grad();
    
    cout << "Sigmoid gradient: " << sigmoid_grad << endl;
    cout << "ReLU gradient: " << relu_grad << endl;

    cout << "\n" << string(60, '=') << endl;
    cout << "DEEP LEARNING NEURAL NETWORK COMPONENTS" << endl;
    cout << string(60, '=') << endl;


    return 0;
}