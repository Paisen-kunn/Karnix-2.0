CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -pthread

all: compare_schedulers

compare_schedulers: compare_schedulers.cpp scheduler.hpp simulator.hpp
	$(CXX) $(CXXFLAGS) -o compare_schedulers compare_schedulers.cpp

run: compare_schedulers
	./compare_schedulers

clean:
	rm -f compare_schedulers scheduler_comparison.csv scheduler_comparison_cpp.png

.PHONY: all run clean
