#include <iostream>
#include <vector>
#include <cmath>

class VectorOperations {
private:
    std::vector<int> vector1;
    std::vector<int> vector2;
    std::vector<int> addResult;
    std::vector<int> subResult;
    std::vector<int> scalarResult;
    std::vector<double> vector_distance;
    double normResult;

public:
    void setVectors(int x[], int sizeX, int y[], int sizeY) {
        vector1.clear();
        vector2.clear();
        
        for(int i = 0; i < sizeX; i++) {
            vector1.push_back(x[i]);
        }
        for(int i = 0; i < sizeY; i++) {
            vector2.push_back(y[i]);
        }
    }

    void displayVector1() {
        std::cout << "Vector 1: ";
        for(int i = 0; i < vector1.size(); i++) {
            std::cout << vector1[i] << " ";
        }
        std::cout << std::endl;
    }
    
    void displayVector2() {
        std::cout << "Vector 2: ";
        for(int i = 0; i < vector2.size(); i++) {
            std::cout << vector2[i] << " ";
        }
        std::cout << std::endl;
    }

    std::vector<int> vector_add() {
        addResult.clear();

        if(vector1.size() != vector2.size()) {
            std::cout << "Error: Vectors must have same size for addition!" << std::endl;
            return addResult;
        }
        
        for(int i = 0; i < vector1.size(); i++) {
            addResult.push_back(vector1[i] + vector2[i]);
        }
        return addResult;
    }

    std::vector<int> vector_sub() {
        subResult.clear();

        if(vector1.size() != vector2.size()) {
            std::cout << "Error: Vectors must have same size for subtraction!" << std::endl;
            return subResult;
        }
        
        for(int i = 0; i < vector1.size(); i++) {
            subResult.push_back(vector1[i] - vector2[i]);
        }
        return subResult;
    }

    std::vector<int> scalarVectorMultiply(int scalar) {
        scalarResult.clear();

        for(int i = 0; i < vector1.size(); i++) {
            scalarResult.push_back(vector1[i] * scalar);
        }
        return scalarResult;
    }

    std::vector<double> vector_Distance() {
        vector_distance.clear();

        if(vector1.size() != vector2.size()) {
            std::cout << "Error: Vectors must have same size to calculate distance!" << std::endl;
            return vector_distance;
        }

        for(int i = 0; i < vector1.size(); i++) {
            double dist = std::abs(vector1[i] - vector2[i]);
            vector_distance.push_back(dist);
        }
        return vector_distance;
    }


    double vector_norm() {
        double sum = 0.0;
        for(int i = 0; i < vector1.size(); i++) {
            sum += vector1[i] * vector1[i];
        }
        normResult = std::sqrt(sum);
        return normResult;
    }


    void displayAddResult() {
        std::cout << "Addition Result: ";
        for(int i = 0; i < addResult.size(); i++) {
            std::cout << addResult[i] << " ";
        }
        std::cout << std::endl;
    }

    void displaySubResult() {
        std::cout << "Subtraction Result: ";
        for (int i = 0; i < subResult.size(); i++) {
            std::cout << subResult[i] << " ";
        }
        std::cout << std::endl;
    }

    void displayScalarResult() {
        std::cout << "Scalar Multiplication Result: ";
        for (int i = 0; i < scalarResult.size(); i++) {
            std::cout << scalarResult[i] << " ";
        }
        std::cout << std::endl;
    }

    void displayNormResult() {
        std::cout << "Vector Norm (Magnitude): " << normResult << std::endl;
    }

    void displayDistanceResult() {
        std::cout << "Vector Distance: ";
        for (int i = 0; i < vector_distance.size(); i++) {
            std::cout << vector_distance[i] << " ";
        }
        std::cout << std::endl;
    }
};