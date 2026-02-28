#include <iostream>

class BasicOpertion
{
private:
    int a, b, c;

public:
    void setValues(int x, int y)
    {
        a = x;
        b = y;
    }
    int add()
    {
        c = a + b;
        return c;
    }
    int sub()
    {
        c = a - b;
        return c;
    }
    int mul()
    {
        c = a * b;
        return c;
    }
    int div()
    {
        try
        {
            if (b == 0) {
                std::cerr << "Error: Division by zero!" << '\n';
                return 0;
            }
            c = a / b;
            return c;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            return 0;
        }
    }
    int sqrt()
    {
        c = a * a;
        return c;
    }
    int abs()
    {
        if (a < 0)
        {
            a = a * -1;
        }
        return a;
    }
};