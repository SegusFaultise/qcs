CXX = g++

TARGET_CLASSICAL = quantum_simulator_classical
CXXFLAGS_CLASSICAL = -Wall -Wextra -std=c++17 -O3

TARGET_PARALLEL = quantum_simulator_parallel
CXXFLAGS_PARALLEL = -Wall -Wextra -std=c++17 -O3 -fopenmp # The only difference is this flag

SRCDIR = src
BUILDDIR = build
SRCS = $(wildcard $(SRCDIR)/*.cpp)

all: classical parallel

classical: $(BUILDDIR)/$(TARGET_CLASSICAL)

$(BUILDDIR)/$(TARGET_CLASSICAL): $(SRCS) | $(BUILDDIR)
	@echo "--- Building Classical Version ---"
	$(CXX) $(CXXFLAGS_CLASSICAL) $(SRCS) -o $@

parallel: $(BUILDDIR)/$(TARGET_PARALLEL)

$(BUILDDIR)/$(TARGET_PARALLEL): $(SRCS) | $(BUILDDIR)
	@echo "--- Building Parallel Version ---"
	$(CXX) $(CXXFLAGS_PARALLEL) $(SRCS) -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run_classical: classical
	./$(BUILDDIR)/$(TARGET_CLASSICAL)

run_parallel: parallel
	./$(BUILDDIR)/$(TARGET_PARALLEL)

clean:
	@echo "Cleaning up..."
	rm -rf $(BUILDDIR)

.PHONY: all classical parallel run_classical run_parallel clean
