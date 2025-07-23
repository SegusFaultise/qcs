CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3 -fopenmp

SRCDIR = src
BUILDDIR = build
TARGET = quantum_simulator

SRCS = $(wildcard $(SRCDIR)/*.cpp)
# Create a list of corresponding object files in the build directory
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))

all: $(BUILDDIR)/$(TARGET)


$(BUILDDIR)/$(TARGET): $(OBJS)
	@echo "Linking..."
	$(CXX) $(OBJS) -o $@ $(CXXFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	@echo "Compiling $<..."
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)


run: all
	./$(BUILDDIR)/$(TARGET)

clean:
	@echo "Cleaning up..."
	rm -rf $(BUILDDIR)

.PHONY: all run clean
