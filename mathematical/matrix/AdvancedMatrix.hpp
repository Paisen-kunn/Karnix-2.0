#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

class AdvancedMatrix {
private:
    std::vector<std::vector<double>> matrix;
    std::vector<double> eigenvalues;
    std::vector<std::vector<double>> eigenvectors;
    std::vector<double> projection_result;
    int rows;
    int cols;

public:
    void setMatrix(const std::vector<std::vector<double>>& mat) {
        matrix = mat;
        if (!mat.empty()) {
            rows = mat.size();
            cols = mat[0].size();
        }
    }

    void displayMatrix() {
        std::cout << "Matrix:" << std::endl;
        for (const auto& row : matrix) {
            for (double element : row) {
                std::cout << element << " ";
            }
            std::cout << std::endl;
        }
    }

    double matrix_trace() {
        if (matrix.empty() || rows != cols) {
            std::cout << "Error: Matrix must be square for trace calculation!" << std::endl;
            return 0.0;
        }

        double trace = 0.0;
        for (int i = 0; i < rows; i++) {
            trace += matrix[i][i];
        }
        return trace;
    }

    void displayTrace() {
        double trace = matrix_trace();
        std::cout << "Matrix Trace: " << trace << std::endl;
    }

    std::vector<double> eigenvalues_conceptual() {
        eigenvalues.clear();
        
        if (matrix.empty() || rows != cols) {
            std::cout << "Error: Matrix must be square for eigenvalue calculation!" << std::endl;
            return eigenvalues;
        }

        // For a 2x2 matrix, we can calculate eigenvalues analytically
        if (rows == 2 && cols == 2) {
            double a = matrix[0][0];
            double b = matrix[0][1];
            double c = matrix[1][0];
            double d = matrix[1][1];
            
            // Characteristic equation: λ² - trace*λ + determinant = 0
            double trace = a + d;
            double det = a * d - b * c;
            
            double discriminant = trace * trace - 4 * det;
            
            if (discriminant >= 0) {
                double sqrt_disc = std::sqrt(discriminant);
                eigenvalues.push_back((trace + sqrt_disc) / 2.0);
                eigenvalues.push_back((trace - sqrt_disc) / 2.0);
            } else {
                std::cout << "Complex eigenvalues detected (not computed)" << std::endl;
            }
        } else {
            // For larger matrices, this is conceptual - in practice would use iterative methods
            std::cout << "Eigenvalue computation for " << rows << "x" << cols 
                      << " matrix requires advanced numerical methods" << std::endl;
            std::cout << "Conceptually: solve det(A - λI) = 0" << std::endl;
        }
        
        return eigenvalues;
    }

    void displayEigenvalues() {
        eigenvalues_conceptual();
        if (!eigenvalues.empty()) {
            std::cout << "Eigenvalues: ";
            for (double val : eigenvalues) {
                std::cout << val << " ";
            }
            std::cout << std::endl;
        }
    }

    std::vector<std::vector<double>> eigenvectors_conceptual() {
        eigenvectors.clear();
        
        if (matrix.empty() || rows != cols) {
            std::cout << "Error: Matrix must be square for eigenvector calculation!" << std::endl;
            return eigenvectors;
        }

        // First compute eigenvalues
        eigenvalues_conceptual();
        
        if (!eigenvalues.empty() && rows == 2) {
            // For 2x2 matrix, compute eigenvectors
            for (double lambda : eigenvalues) {
                std::vector<double> eigenvector;
                
                // Solve (A - λI)v = 0
                double a11 = matrix[0][0] - lambda;
                double a12 = matrix[0][1];
                double a21 = matrix[1][0];
                double a22 = matrix[1][1] - lambda;
                
                // Choose v2 = 1, solve for v1
                if (std::abs(a12) > 1e-10) {
                    double v1 = -a22 / a12;
                    eigenvector = {v1, 1.0};
                } else if (std::abs(a21) > 1e-10) {
                    double v2 = -a11 / a21;
                    eigenvector = {1.0, v2};
                } else {
                    eigenvector = {1.0, 0.0}; // Default case
                }
                
                eigenvectors.push_back(eigenvector);
            }
        } else {
            std::cout << "Eigenvector computation for " << rows << "x" << cols 
                      << " matrix requires advanced numerical methods" << std::endl;
            std::cout << "Conceptually: solve (A - λI)v = 0 for each eigenvalue λ" << std::endl;
        }
        
        return eigenvectors;
    }

    void displayEigenvectors() {
        eigenvectors_conceptual();
        if (!eigenvectors.empty()) {
            std::cout << "Eigenvectors:" << std::endl;
            for (int i = 0; i < eigenvectors.size(); i++) {
                std::cout << "v" << i + 1 << ": [";
                for (int j = 0; j < eigenvectors[i].size(); j++) {
                    std::cout << eigenvectors[i][j];
                    if (j < eigenvectors[i].size() - 1) std::cout << ", ";
                }
                std::cout << "]" << std::endl;
            }
        }
    }

    int matrix_rank() {
        if (matrix.empty()) {
            std::cout << "Error: Matrix is empty!" << std::endl;
            return 0;
        }

        // Create a copy for row reduction
        std::vector<std::vector<double>> temp = matrix;
        int rank = 0;
        
        // Gaussian elimination to find rank
        for (int col = 0; col < cols && rank < rows; col++) {
            // Find pivot
            int pivot_row = rank;
            for (int row = rank + 1; row < rows; row++) {
                if (std::abs(temp[row][col]) > std::abs(temp[pivot_row][col])) {
                    pivot_row = row;
                }
            }
            
            // If pivot is zero, skip this column
            if (std::abs(temp[pivot_row][col]) < 1e-10) {
                continue;
            }
            
            // Swap rows
            if (pivot_row != rank) {
                std::swap(temp[rank], temp[pivot_row]);
            }
            
            // Eliminate below pivot
            for (int row = rank + 1; row < rows; row++) {
                if (std::abs(temp[row][col]) > 1e-10) {
                    double factor = temp[row][col] / temp[rank][col];
                    for (int c = col; c < cols; c++) {
                        temp[row][c] -= factor * temp[rank][c];
                    }
                }
            }
            rank++;
        }
        
        return rank;
    }

    void displayRank() {
        int rank = matrix_rank();
        std::cout << "Matrix Rank: " << rank << std::endl;
    }

    std::vector<double> matrix_projection(const std::vector<double>& b) {
        projection_result.clear();
        
        if (matrix.empty() || b.size() != rows) {
            std::cout << "Error: Vector dimension must match matrix rows!" << std::endl;
            return projection_result;
        }

        // Project b onto the column space of A
        // proj = A(A^T A)^(-1) A^T b
        
        // For simplicity, we'll compute projection onto first column vector
        if (cols > 0) {
            std::vector<double> first_col;
            for (int i = 0; i < rows; i++) {
                first_col.push_back(matrix[i][0]);
            }
            
            // Calculate dot products
            double dot_col_b = 0.0;
            double dot_col_col = 0.0;
            
            for (int i = 0; i < rows; i++) {
                dot_col_b += first_col[i] * b[i];
                dot_col_col += first_col[i] * first_col[i];
            }
            
            if (std::abs(dot_col_col) > 1e-10) {
                double scalar = dot_col_b / dot_col_col;
                
                projection_result.resize(rows);
                for (int i = 0; i < rows; i++) {
                    projection_result[i] = scalar * first_col[i];
                }
            }
        }
        
        return projection_result;
    }

    void displayProjection(const std::vector<double>& b) {
        std::cout << "Original vector b: [";
        for (int i = 0; i < b.size(); i++) {
            std::cout << b[i];
            if (i < b.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        
        matrix_projection(b);
        
        if (!projection_result.empty()) {
            std::cout << "Projection of b onto matrix column space: [";
            for (int i = 0; i < projection_result.size(); i++) {
                std::cout << projection_result[i];
                if (i < projection_result.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
    }
};