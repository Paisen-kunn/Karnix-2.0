#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iomanip>

class GraphVisualizer {
private:
    int width;
    int height;
    std::vector<std::vector<char>> canvas;
    double x_min, x_max, y_min, y_max;
    std::string title;

public:
    GraphVisualizer(int w = 80, int h = 25) : width(w), height(h) {
        canvas.resize(height, std::vector<char>(width, ' '));
        x_min = -10.0; x_max = 10.0;
        y_min = -10.0; y_max = 10.0;
    }

    void setRange(double xmin, double xmax, double ymin, double ymax) {
        x_min = xmin; x_max = xmax;
        y_min = ymin; y_max = ymax;
    }

    void setTitle(const std::string& t) {
        title = t;
    }

    void clearCanvas() {
        for (auto& row : canvas) {
            std::fill(row.begin(), row.end(), ' ');
        }
    }

    std::pair<int, int> worldToScreen(double x, double y) {
        int screen_x = (int)((x - x_min) / (x_max - x_min) * (width - 1));
        int screen_y = (int)((y_max - y) / (y_max - y_min) * (height - 1));
        
        screen_x = std::max(0, std::min(width - 1, screen_x));
        screen_y = std::max(0, std::min(height - 1, screen_y));
        
        return {screen_x, screen_y};
    }

    void drawAxes() {
        // Draw horizontal axis (y = 0)
        if (y_min <= 0 && y_max >= 0) {
            std::pair<int, int> axis_start = worldToScreen(x_min, 0);
            std::pair<int, int> axis_end = worldToScreen(x_max, 0);
            int x_axis_start = axis_start.first;
            int y_axis_pos = axis_start.second;
            int x_axis_end = axis_end.first;
            
            for (int x = x_axis_start; x <= x_axis_end; x++) {
                if (x >= 0 && x < width && y_axis_pos >= 0 && y_axis_pos < height) {
                    canvas[y_axis_pos][x] = '-';
                }
            }
        }

        // Draw vertical axis (x = 0)
        if (x_min <= 0 && x_max >= 0) {
            std::pair<int, int> axis_start = worldToScreen(0, y_max);
            std::pair<int, int> axis_end = worldToScreen(0, y_min);
            int x_axis_pos = axis_start.first;
            int y_axis_start = axis_start.second;
            int y_axis_end = axis_end.second;
            
            for (int y = y_axis_start; y <= y_axis_end; y++) {
                if (y >= 0 && y < height && x_axis_pos >= 0 && x_axis_pos < width) {
                    canvas[y][x_axis_pos] = '|';
                }
            }
        }

        // Mark origin
        std::pair<int, int> origin = worldToScreen(0, 0);
        int origin_x = origin.first;
        int origin_y = origin.second;
        if (origin_x >= 0 && origin_x < width && origin_y >= 0 && origin_y < height) {
            canvas[origin_y][origin_x] = '+';
        }
    }

    void plotFunction(std::function<double(double)> func, char symbol = '*') {
        for (int screen_x = 0; screen_x < width; screen_x++) {
            double world_x = x_min + (double)screen_x / (width - 1) * (x_max - x_min);
            double world_y = func(world_x);
            
            std::pair<int, int> screen_pos = worldToScreen(world_x, world_y);
            int sx = screen_pos.first;
            int sy = screen_pos.second;
            if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                canvas[sy][sx] = symbol;
            }
        }
    }

    void plotPoints(const std::vector<std::pair<double, double>>& points, char symbol = 'o') {
        for (const auto& point : points) {
            std::pair<int, int> screen_pos = worldToScreen(point.first, point.second);
            int sx = screen_pos.first;
            int sy = screen_pos.second;
            if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                canvas[sy][sx] = symbol;
            }
        }
    }

    void plotVector(double start_x, double start_y, double end_x, double end_y, char symbol = '>') {
        std::pair<int, int> start_pos = worldToScreen(start_x, start_y);
        std::pair<int, int> end_pos = worldToScreen(end_x, end_y);
        int sx1 = start_pos.first;
        int sy1 = start_pos.second;
        int sx2 = end_pos.first;
        int sy2 = end_pos.second;
        
        // Simple line drawing using Bresenham-like algorithm
        int dx = abs(sx2 - sx1);
        int dy = abs(sy2 - sy1);
        int x_step = (sx1 < sx2) ? 1 : -1;
        int y_step = (sy1 < sy2) ? 1 : -1;
        int error = dx - dy;
        
        int x = sx1, y = sy1;
        while (true) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                canvas[y][x] = symbol;
            }
            
            if (x == sx2 && y == sy2) break;
            
            int error2 = 2 * error;
            if (error2 > -dy) {
                error -= dy;
                x += x_step;
            }
            if (error2 < dx) {
                error += dx;
                y += y_step;
            }
        }
    }

    void addScale() {
        // Add scale markers on axes
        int num_ticks = 5;
        
        // X-axis scale
        if (y_min <= 0 && y_max >= 0) {
            std::pair<int, int> axis_pos = worldToScreen(0, 0);
            int y_pos = axis_pos.second;
            for (int i = 0; i <= num_ticks; i++) {
                double x_val = x_min + i * (x_max - x_min) / num_ticks;
                std::pair<int, int> tick_pos = worldToScreen(x_val, 0);
                int x_pos = tick_pos.first;
                if (x_pos >= 0 && x_pos < width && y_pos + 1 < height) {
                    canvas[y_pos + 1][x_pos] = '|';
                }
            }
        }
        
        // Y-axis scale
        if (x_min <= 0 && x_max >= 0) {
            std::pair<int, int> axis_pos = worldToScreen(0, 0);
            int x_pos = axis_pos.first;
            for (int i = 0; i <= num_ticks; i++) {
                double y_val = y_min + i * (y_max - y_min) / num_ticks;
                std::pair<int, int> tick_pos = worldToScreen(0, y_val);
                int y_pos = tick_pos.second;
                if (y_pos >= 0 && y_pos < height && x_pos + 1 < width) {
                    canvas[y_pos][x_pos + 1] = '-';
                }
            }
        }
    }

    void display() {
        // Display title
        if (!title.empty()) {
            std::cout << std::string((width - title.length()) / 2, ' ') << title << std::endl;
            std::cout << std::string(width, '=') << std::endl;
        }
        
        // Display canvas
        for (const auto& row : canvas) {
            for (char c : row) {
                std::cout << c;
            }
            std::cout << std::endl;
        }
        
        // Display scale information
        std::cout << std::string(width, '=') << std::endl;
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "X: [" << x_min << " to " << x_max << "] ";
        std::cout << "Y: [" << y_min << " to " << y_max << "]" << std::endl;
    }

    // Specialized plotting functions
    void plotSineWave() {
        setTitle("Sine Wave: y = sin(x)");
        setRange(-2*M_PI, 2*M_PI, -1.5, 1.5);
        clearCanvas();
        drawAxes();
        plotFunction([](double x) { return std::sin(x); }, '*');
        addScale();
        display();
    }

    void plotQuadratic() {
        setTitle("Quadratic Function: y = x²");
        setRange(-5, 5, 0, 25);
        clearCanvas();
        drawAxes();
        plotFunction([](double x) { return x * x; }, '*');
        addScale();
        display();
    }

    void plotExponential() {
        setTitle("Exponential Function: y = e^x");
        setRange(-3, 3, 0, 20);
        clearCanvas();
        drawAxes();
        plotFunction([](double x) { return std::exp(x); }, '*');
        addScale();
        display();
    }

    void plotLogarithmic() {
        setTitle("Natural Logarithm: y = ln(x)");
        setRange(0.1, 10, -3, 3);
        clearCanvas();
        drawAxes();
        plotFunction([](double x) { return std::log(x); }, '*');
        addScale();
        display();
    }

    void plotDerivative() {
        setTitle("Function and its Derivative: f(x)=x³-3x, f'(x)=3x²-3");
        setRange(-3, 3, -5, 5);
        clearCanvas();
        drawAxes();
        plotFunction([](double x) { return x*x*x - 3*x; }, '*');  // Original function
        plotFunction([](double x) { return 3*x*x - 3; }, '#');    // Derivative
        addScale();
        display();
        std::cout << "Legend: * = f(x), # = f'(x)" << std::endl;
    }

    void plotGradientField() {
        setTitle("Gradient Field: f(x,y) = x² + y²");
        setRange(-3, 3, -3, 3);
        clearCanvas();
        drawAxes();
        
        // Plot gradient vectors
        for (double x = -2.5; x <= 2.5; x += 0.5) {
            for (double y = -2.5; y <= 2.5; y += 0.5) {
                double grad_x = 2 * x;  // ∂f/∂x = 2x
                double grad_y = 2 * y;  // ∂f/∂y = 2y
                
                // Scale the gradient for visualization
                double scale = 0.2;
                plotVector(x, y, x + scale * grad_x, y + scale * grad_y, '>');
            }
        }
        
        addScale();
        display();
        std::cout << "Gradient vectors showing direction of steepest ascent" << std::endl;
    }

    void plotContour() {
        setTitle("Contour Plot: f(x,y) = x² + y² (circular contours)");
        setRange(-5, 5, -5, 5);
        clearCanvas();
        drawAxes();
        
        // Plot contour lines for different levels
        std::vector<double> levels = {1, 4, 9, 16};
        std::vector<char> symbols = {'o', '+', 'x', '#'};
        
        for (int level_idx = 0; level_idx < levels.size(); level_idx++) {
            double level = levels[level_idx];
            char symbol = symbols[level_idx];
            
            // Plot points where f(x,y) ≈ level
            for (double angle = 0; angle < 2 * M_PI; angle += 0.1) {
                double radius = std::sqrt(level);
                double x = radius * std::cos(angle);
                double y = radius * std::sin(angle);
                
                std::pair<int, int> screen_pos = worldToScreen(x, y);
                int sx = screen_pos.first;
                int sy = screen_pos.second;
                if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                    canvas[sy][sx] = symbol;
                }
            }
        }
        
        addScale();
        display();
        std::cout << "Contour levels: o=1, +=4, x=9, #=16" << std::endl;
    }

    void plotDataScatter() {
        setTitle("Scatter Plot: Sample Data Points");
        setRange(0, 10, 0, 10);
        clearCanvas();
        drawAxes();
        
        // Generate sample data
        std::vector<std::pair<double, double>> data = {
            {1, 2}, {2, 3}, {3, 5}, {4, 4}, {5, 6},
            {6, 7}, {7, 6}, {8, 8}, {9, 9}, {2, 1}
        };
        
        plotPoints(data, 'o');
        
        // Add trend line (simple linear regression)
        double sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
        for (const auto& point : data) {
            sum_x += point.first;
            sum_y += point.second;
            sum_xy += point.first * point.second;
            sum_xx += point.first * point.first;
        }
        
        double n = data.size();
        double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
        double intercept = (sum_y - slope * sum_x) / n;
        
        plotFunction([slope, intercept](double x) { return slope * x + intercept; }, '-');
        
        addScale();
        display();
        std::cout << "Trend line: y = " << std::fixed << std::setprecision(2) 
                  << slope << "x + " << intercept << std::endl;
    }
};