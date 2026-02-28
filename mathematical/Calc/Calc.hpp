#include <iostream>
#include <vector>
#include <cmath>

class Calc
{
public:
    double constant(double x, double c)
    {
        return c;
    }
    double constant_derivative(double x)
    {
        return 0.0;
    }
    double linear(double x, double m, double b)
    {
        return m * x + b;
    }
    double polynomial(double x, const std::vector<double> &coeffs)
    {
        double result = 0.0;
        double power = 1.0;
        for (double coef : coeffs)
        {
            result += coef * power;
            power *= x;
        }
        return result;
    }
    double polynomial_derivative(double x, const std::vector<double> &coeffs)
    {
        double result = 0.0;
        double power = 1.0;
        for (size_t i = 1; i < coeffs.size(); i++)
        {
            result += i * coeffs[i] * power;
            power *= x;
        }
        return result;
    }
    double exponential(double x)
    {
        return exp(x);
    }
    double exponential_derivative(double x)
    {
        return exp(x);
    }

    double logarithmic(double x)
    {
        return log(x);
    }
    double logarithmic_derivative(double x)
    {
        return 1.0 / x;
    }

    // Trigonometric functions
    double sine(double x)
    {
        return sin(x);
    }
    double sine_derivative(double x)
    {
        return cos(x);
    }
    double cosine(double x)
    {
        return cos(x);
    }
    double cosine_derivative(double x)
    {
        return -sin(x);
    }

    double g(double x)
    {
        return x + 1;
    }
    double g_derivative(double x)
    {
        return 1;
    }
    double f_of_g(double x)
    {
        double gx = g(x);
        return gx * gx;
    }
    double composite_derivative(double x)
    {
        return 2 * g(x) * g_derivative(x);
    }

    double multivariate(double x, double y)
    {
        return x * x + y * y;
    }
    std::pair<double, double> multivariate_gradient(double x, double y)
    {
        return {2 * x, 2 * y};
    }
};