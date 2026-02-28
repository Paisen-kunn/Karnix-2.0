#include <iostream>
#include <vector>
#include <cmath>

class MatrixOperations {
private:
    std::vector<std::vector<int>> matrix1;
    std::vector<std::vector<int>> matrix2;
    std::vector<std::vector<int>> result;
    std::vector<std::vector<int>> scalarResult;
    int rows;
    int cols;

public:
    void setMatrices(const std::vector<std::vector<int>>& mat1, const std::vector<std::vector<int>>& mat2) {
        matrix1 = mat1;
        matrix2 = mat2;
        if (!mat1.empty()) {
            rows = mat1.size();
            cols = mat1[0].size();
        }
    }

    void setMatrix(const std::vector<std::vector<int>>& mat) {
        matrix1 = mat;
        if (!mat.empty()) {
            rows = mat.size();
            cols = mat[0].size();
        }
    }

    void displayMatrix1() {
        std::cout << "Matrix 1:" << std::endl;
        for (const auto& row : matrix1) {
            for (int element : row) {
                std::cout << element << " ";
            }
            std::cout << std::endl;
        }
    }

    void displayMatrix2() {
        std::cout << "Matrix 2:" << std::endl;
        for (const auto& row : matrix2) {
            for (int element : row) {
                std::cout << element << " ";
            }
            std::cout << std::endl;
        }
    }

    std::vector<std::vector<int>> matrix_add() {
        result.clear();
        
        if (matrix1.size() != matrix2.size() || 
            (matrix1.size() > 0 && matrix1[0].size() != matrix2[0].size())) {
            std::cout << "Error: Matrices must have same dimensions for addition!" << std::endl;
            return result;
        }

        result.resize(matrix1.size(), std::vector<int>(matrix1[0].size()));
        
        for (int i = 0; i < matrix1.size(); i++) {
            for (int j = 0; j < matrix1[i].size(); j++) {
                result[i][j] = matrix1[i][j] + matrix2[i][j];
            }
        }
        return result;
    }

    std::vector<std::vector<int>> matrix_sub() {
        result.clear();
        
        if (matrix1.size() != matrix2.size() || 
            (matrix1.size() > 0 && matrix1[0].size() != matrix2[0].size())) {
            std::cout << "Error: Matrices must have same dimensions for subtraction!" << std::endl;
            return result;
        }

        result.resize(matrix1.size(), std::vector<int>(matrix1[0].size()));
        
        for (int i = 0; i < matrix1.size(); i++) {
            for (int j = 0; j < matrix1[i].size(); j++) {
                result[i][j] = matrix1[i][j] - matrix2[i][j];
            }
        }
        return result;
    }

    std::vector<std::vector<int>> matrix_multiply() {
        result.clear();
        
        if (matrix1.empty() || matrix2.empty() || matrix1[0].size() != matrix2.size()) {
            std::cout << "Error: Matrix1 columns must equal Matrix2 rows for multiplication!" << std::endl;
            return result;
        }

        int rows1 = matrix1.size();
        int cols1 = matrix1[0].size();
        int cols2 = matrix2[0].size();
        
        result.resize(rows1, std::vector<int>(cols2, 0));
        
        for (int i = 0; i < rows1; i++) {
            for (int j = 0; j < cols2; j++) {
                for (int k = 0; k < cols1; k++) {
                    result[i][j] += matrix1[i][k] * matrix2[k][j];
                }
            }
        }
        return result;
    }

    std::vector<std::vector<int>> scalar_multiply(int scalar) {
        scalarResult.clear();
        
        if (matrix1.empty()) {
            std::cout << "Error: Matrix is empty!" << std::endl;
            return scalarResult;
        }

        scalarResult.resize(matrix1.size(), std::vector<int>(matrix1[0].size()));
        
        for (int i = 0; i < matrix1.size(); i++) {
            for (int j = 0; j < matrix1[i].size(); j++) {
                scalarResult[i][j] = matrix1[i][j] * scalar;
            }
        }
        return scalarResult;
    }

    std::vector<std::vector<int>> matrix_transpose() {
        result.clear();
        
        if (matrix1.empty()) {
            std::cout << "Error: Matrix is empty!" << std::endl;
            return result;
        }

        int rows = matrix1.size();
        int cols = matrix1[0].size();
        result.resize(cols, std::vector<int>(rows));
        
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[j][i] = matrix1[i][j];
            }
        }
        return result;
    }

    double matrix_norm() {
        if (matrix1.empty()) {
            std::cout << "Error: Matrix is empty!" << std::endl;
            return 0.0;
        }

        double sum = 0.0;
        for (const auto& row : matrix1) {
            for (int element : row) {
                sum += element * element;
            }
        }
        return std::sqrt(sum);
    }

    void displayAddResult() {
        std::cout << "Addition Result:" << std::endl;
        for (const auto& row : result) {
            for (int element : row) {
                std::cout << element << " ";
            }
            std::cout << std::endl;
        }
    }

    void displaySubResult() {
        std::cout << "Subtraction Result:" << std::endl;
        for (const auto& row : result) {
            for (int element : row) {
                std::cout << element << " ";
            }
            std::cout << std::endl;
        }
    }

    void displayMultiplyResult() {
        std::cout << "Multiplication Result:" << std::endl;
        for (const auto& row : result) {
            for (int element : row) {
                std::cout << element << " ";
            }
            std::cout << std::endl;
        }
    }

    void displayScalarResult() {
        std::cout << "Scalar Multiplication Result:" << std::endl;
        for (const auto& row : scalarResult) {
            for (int element : row) {
                std::cout << element << " ";
            }
            std::cout << std::endl;
        }
    }

    void displayTransposeResult() {
        std::cout << "Transpose Result:" << std::endl;
        for (const auto& row : result) {
            for (int element : row) {
                std::cout << element << " ";
            }
            std::cout << std::endl;
        }
    }

    void displayNorm() {
        double norm = matrix_norm();
        std::cout << "Matrix Norm (Frobenius): " << norm << std::endl;
    }
};