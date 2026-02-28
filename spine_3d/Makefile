##
##  KarnixOS – Spine 3D Viewer
##  macOS build with OpenGL 2.1, GLFW3, GLUT (fonts), libpq
##

CXX    := clang++
TARGET := spine3d
SRC    := spine_3d_viewer.cpp

GLFW_PREFIX := $(shell brew --prefix glfw  2>/dev/null || echo /opt/homebrew/opt/glfw)
LIBPQ_PREFIX:= $(shell brew --prefix libpq 2>/dev/null || echo /opt/homebrew/opt/libpq)

CXXFLAGS := \
    -std=c++17 -O2 \
    -Wall -Wno-deprecated-declarations \
    -I$(GLFW_PREFIX)/include \
    -I$(LIBPQ_PREFIX)/include

LDFLAGS := \
    -L$(GLFW_PREFIX)/lib \
    -L$(LIBPQ_PREFIX)/lib \
    -lglfw -lpq \
    -framework OpenGL \
    -framework GLUT \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreFoundation

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)
	@echo ""
	@echo "  Build successful -> ./$(TARGET)"
	@echo "  Run with: make run"
	@echo ""

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
