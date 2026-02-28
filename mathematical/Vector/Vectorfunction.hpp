#include <iostream>
#include <vector>

class VectorOperations {
private:
    std::vector<int> vectorVar;
    std::vector<int> vector1;
    std::vector<int> vector2;
    std::vector<int> addresult;
    std::vector<int> subResult;
    std::vector<int> scalarResult;
    std::vector<int> DotProduct;

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
        addresult.clear();

        if(vector1.size() != vector2.size()) {
            std::cout << "Error: Vectors must have same size for addition!" << std::endl;
            return addresult;
        }
        
        for(int i = 0; i < vector1.size(); i++) {
            addresult.push_back(vector1[i] + vector2[i]);
        }
        return addresult;
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

    std::vector<int> dotProduct() {
        DotProduct.clear();

        if(vector1.size() != vector2.size()) {
            std::cout << "Error: Vectors must have same size for dot product!" << std::endl;
            return DotProduct;
        }

        int dotProd = 0;
        for(int i = 0; i < vector1.size(); i++) {
            dotProd += vector1[i] * vector2[i];
        }
        DotProduct.push_back(dotProd);
        return DotProduct;
    }

    void displayResultadd() {
        std::cout << "Addition Result: ";
        for(int i = 0; i < addresult.size(); i++) {
            std::cout << addresult[i] << " ";
        }
        std::cout << std::endl;
    }

    void displayResultsub() {
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

    void displayMultiply(){
        std::cout << "Scalar Multiplication Result: ";
        for (int i = 0; i < scalarResult.size(); i++) {
            std::cout << scalarResult[i] << " ";
        }
        std::cout << std::endl;
    }

    void displayDotProduct() {
        std::cout << "Dot Product Result: ";
        for (int i = 0; i < DotProduct.size(); i++) {
            std::cout << DotProduct[i] << " ";
        }
        std::cout << std::endl;
    }
};