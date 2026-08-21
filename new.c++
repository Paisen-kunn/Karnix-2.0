#pragma once

#include <cstddef>
#include <initializer_list>
#include <vector>
#include <iostream>

namespace sciforge::math{
    class vector{
        public:
            vector() = default;

            void pushback(double value){
                data.push_back(value);
            }

            double at(std::size_t index) const {
                return data.at(index);
            }

        private:
            std::vector<double> data;
    };
}


using namespace std;


int main(){
    sciforge::math::vector vectorwork; //creating a instance
    sciforge::math::vector vectorwork2; //creating a instance

    vectorwork.pushback(10.5);
    vectorwork.pushback(20.5);
    cout << "vector values " << vectorwork.at(0) << endl;
    cout<< "vector values " << vectorwork.at(1) << endl;

    cout<< "vector address" << &vectorwork << endl;
    cout << "vector address 2" << &vectorwork2 << endl;
    return 0;

}

