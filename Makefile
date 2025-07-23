# Compiler
CXX = g++
NVCC = nvcc # NVIDIA's CUDA Compiler

# --- Directories ---
SRCDIR = src
BUILDDIR = build
INCLUDEDIR = include

# --- Source File Discovery ---
# Source files common to all builds
COMMON_SRCS = $(SRCDIR)/main.cpp $(SRCDIR)/benchmark.cpp $(SRCDIR)/simulation_options.cpp $(SRCDIR)/state.cpp
# Source files for CPU builds (classical and parallel)
CPU_SRCS = $(SRCDIR)/gates.cpp $(SRCDIR)/circuit_runner.cpp
# Source files for GPU build
GPU_SRCS_CPP = $(SRCDIR)/circuit_runner_gpu.cpp
GPU_SRCS_CU = $(SRCDIR)/gates_cuda.cu

# --- Object File Lists ---
COMMON_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/common_%.o,$(COMMON_SRCS))
CLASSICAL_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/classical_%.o,$(CPU_SRCS))
PARALLEL_CPU_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/parallel_cpu_%.o,$(CPU_SRCS))
GPU_OBJS_CPP = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/gpu_%.o,$(GPU_SRCS_CPP))
GPU_OBJS_CU = $(patsubst $(SRCDIR)/%.cu,$(BUILDDIR)/gpu_%.o,$(GPU_SRCS_CU))

# --- Target Executables ---
TARGET_CLASSICAL = quantum_simulator_classical
TARGET_PARALLEL_CPU = quantum_simulator_parallel_cpu
TARGET_PARALLEL_GPU = quantum_simulator_parallel_gpu

# --- Compiler Flags ---
CXXFLAGS = -I$(INCLUDEDIR) -Wall -Wextra -std=c++17 -O3
CXXFLAGS_PARALLEL_CPU = $(CXXFLAGS) -fopenmp
NVCCFLAGS_GPU = -I$(INCLUDEDIR) -O3 -std=c++17 -arch=sm_60

# --- Build Targets ---
all: $(BUILDDIR)/$(TARGET_CLASSICAL) $(BUILDDIR)/$(TARGET_PARALLEL_CPU)

classical: $(BUILDDIR)/$(TARGET_CLASSICAL)
parallel_cpu: $(BUILDDIR)/$(TARGET_PARALLEL_CPU)
parallel_gpu: $(BUILDDIR)/$(TARGET_PARALLEL_GPU)

$(BUILDDIR)/$(TARGET_CLASSICAL): $(COMMON_OBJS) $(CLASSICAL_OBJS)
	@echo "--- Linking Classical Version ---"
	$(CXX) $^ -o $@ $(CXXFLAGS)

$(BUILDDIR)/$(TARGET_PARALLEL_CPU): $(COMMON_OBJS) $(PARALLEL_CPU_OBJS)
	@echo "--- Linking Parallel CPU Version ---"
	$(CXX) $^ -o $@ $(CXXFLAGS_PARALLEL_CPU)

$(BUILDDIR)/$(TARGET_PARALLEL_GPU): $(COMMON_OBJS) $(GPU_OBJS_CPP) $(GPU_OBJS_CU)
	@echo "--- Linking Parallel GPU Version (requires nvcc) ---"
	$(NVCC) $^ -o $@

# --- Compilation Rules ---
$(BUILDDIR)/common_%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BUILDDIR)/classical_%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BUILDDIR)/parallel_cpu_%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS_PARALLEL_CPU) -c $< -o $@
$(BUILDDIR)/gpu_%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BUILDDIR)/gpu_%.o: $(SRCDIR)/%.cu | $(BUILDDIR)
	$(NVCC) $(NVCCFLAGS_GPU) -c $< -o $@

# --- Utility ---
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	@echo "Cleaning up..."
	rm -rf $(BUILDDIR)

# --- Run and Benchmark Targets (Restored) ---
run_classical: classical
	./$(BUILDDIR)/$(TARGET_CLASSICAL)

run_parallel_cpu: parallel_cpu
	./$(BUILDDIR)/$(TARGET_PARALLEL_CPU)

run_parallel_gpu: parallel_gpu
	./$(BUILDDIR)/$(TARGET_PARALLEL_GPU)

benchmark: classical parallel_cpu
	@echo "\n--- Running Benchmark: Classical ---"
	./$(BUILDDIR)/$(TARGET_CLASSICAL)
	@echo "\n--- Running Benchmark: Parallel CPU ---"
	./$(BUILDDIR)/$(TARGET_PARALLEL_CPU)

.PHONY: all clean classical parallel_cpu parallel_gpu run_classical run_parallel_cpu run_parallel_gpu benchmark
