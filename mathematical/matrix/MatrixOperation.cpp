#include <iostream>
#include <vector>
#include "MatrixOperations.hpp"

int main() {
    MatrixOperations mo;
    
    // Create sample matrices
    std::vector<std::vector<int>> mat1 = {
        {1, 2, 3},
        {4, 5, 6}
    };
    
    std::vector<std::vector<int>> mat2 = {
        {7, 8, 9},
        {10, 11, 12}
    };
    
    std::vector<std::vector<int>> mat3 = {
        {1, 2},
        {3, 4},
        {5, 6}
    };
    
    std::cout << "=== Matrix Operations Demo ===" << std::endl;
    
    // Set matrices for addition/subtraction
    mo.setMatrices(mat1, mat2);
    mo.displayMatrix1();
    mo.displayMatrix2();
    
    // Matrix Addition
    mo.matrix_add();
    mo.displayAddResult();
    
    // Matrix Subtraction
    mo.matrix_sub();
    mo.displaySubResult();
    
    // Matrix Multiplication (mat1 * mat3)
    std::cout << "\n=== Matrix Multiplication ===" << std::endl;
    mo.setMatrices(mat1, mat3);
    mo.displayMatrix1();
    mo.displayMatrix2();
    mo.matrix_multiply();
    mo.displayMultiplyResult();
    
    // Scalar Multiplication
    std::cout << "\n=== Scalar Multiplication ===" << std::endl;
    mo.setMatrix(mat1);
    mo.displayMatrix1();
    mo.scalar_multiply(3);
    mo.displayScalarResult();
    
    // Transpose
    std::cout << "\n=== Matrix Transpose ===" << std::endl;
    mo.matrix_transpose();
    mo.displayTransposeResult();
    
    // Matrix Norm
    std::cout << "\n=== Matrix Norm ===" << std::endl;
    mo.displayNorm();
    
    return 0;
}