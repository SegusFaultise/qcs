# Compiler to use
CXX = g++

# Compiler flags
# -Wall -Wextra: Enable common warnings for better code quality
# -std=c++17: Use C++17 standard features
# -O3: Enable aggressive optimizations (important for performance benchmarking)
# -fopenmp: Enable OpenMP support (CRUCIAL for your parallelization project)
CXXFLAGS = -Wall -Wextra -std=c++17 -O3 -fopenmp

# Source directory
SRCDIR = src

# Build directory for compiled executables
BUILDDIR = build

# Name of the executable
TARGET = quantum_simulator

# Default target: build the executable
all: $(BUILDDIR)/$(TARGET)

# Rule to create the build directory if it doesn't exist
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Rule to compile the main source file into the executable
$(BUILDDIR)/$(TARGET): $(SRCDIR)/main.cpp $(BUILDDIR)
	$(CXX) $(SRCDIR)/main.cpp -o $(BUILDDIR)/$(TARGET) $(CXXFLAGS)

# Rule to run the compiled executable
run: $(BUILDDIR)/$(TARGET)
	./$(BUILDDIR)/$(TARGET)

# Rule to clean up compiled files and the build directory
clean:
	rm -f $(BUILDDIR)/*
	rmdir $(BUILDDIR) 2>/dev/null || true # Remove dir only if empty, suppress error if not

.PHONY: all run clean
