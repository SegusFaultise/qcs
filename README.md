<div align="center">

  <h1>QCS</h1>
  
  <p>
    <strong>A high-performance, C89-compliant quantum circuit simulator with GPU acceleration and complete parallelization support.</strong>
    <br />
    QCS provides quantum logic gates, fundamental algorithms, and multiple parallelization modes (Sequential, Multi-threading, OpenMP, SIMD, GPU) for maximum performance across different hardware architectures.
  </p>

  <p>
    <a href="https://en.wikipedia.org/wiki/C89_(C_standard)"><img src="https://img.shields.io/badge/C_Standard-C89_|_ANSI_C-blue?style=flat&logo=c&logoColor=white" alt="C Standard"></a>
    <a href="https://github.com/SegusFaultise/qcs/releases"><img src="https://img.shields.io/github/v/release/SegusFaultise/qcs" alt="Latest Release"></a>
    <a href="https://github.com/SegusFaultise/qcs/blob/master/LICENSE"><img src="https://img.shields.io/github/license/SegusFaultise/qcs" alt="License"></a>
    <a href="https://github.com/SegusFaultise/qcs/graphs/contributors"><img src="https://img.shields.io/github/contributors/SegusFaultise/qcs" alt="Contributors"></a>
  </p>
</div>

---

## What is QCS?

QCS (Quantum Circuit Simulator) is a high-performance library designed for classical simulation of quantum circuits with **GPU acceleration** and **multiple parallelization modes**. Unlike larger frameworks, QCS is engineered for maximum portability and minimal dependencies, targeting the **C89 standard** to ensure compatibility with nearly any C compiler and system architecture.

The library uses the **state vector simulation** model with **persistent GPU memory** and **optimized OpenCL kernels** for quantum gate operations. It provides **complete parallelization support** including Sequential, Multi-threading (pthread), OpenMP, SIMD (AVX2), and GPU (OpenCL) modes for maximum performance across different hardware.

### **Performance Highlights**
- **GPU Acceleration**: 1.5x speedup with OpenCL optimization
- **Complete Documentation**: 50+ functions with JSDoc-style comments
- **Multiple Parallelization Modes**: Choose the best for your hardware
- **Professional Code Quality**: Clean, maintainable, production-ready code

### Single-Header Distribution

The library is distributed as a single file, **`qcs.h`**, making integration into your project as simple as dropping the file into your source tree and adding two lines of code. No complex build systems or installation steps required for users.

---

## Key Features

* **GPU Acceleration**: OpenCL-based GPU acceleration with persistent memory and optimized kernels
* **Multiple Parallelization Modes**: Sequential, Multi-threading, OpenMP, SIMD (AVX2), and GPU support
* **Complete Documentation**: 50+ functions with JSDoc-style comments and parameter descriptions
* **Single-File Integration**: Delivered entirely in one header file (`qcs.h`) for zero-dependency integration
* **C89 / ANSI C Compliant**: Designed for maximum compatibility with legacy systems
* **Core Gate Set**: Complete quantum gate library (Hadamard, Pauli gates, CNOT, rotations, phase gates)
* **Advanced Algorithms**: Grover's Search, Bernstein-Vazirani, Quantum Fourier Transform
* **State Vector Simulation**: Full quantum state simulation with precise probability measurement
* **High Performance**: Optimized for different hardware architectures with automatic device detection

---

## Getting Started

### Prerequisites

You only need a standard C compiler and the ability to link against the math library (`-lm`).

* **C Compiler**: GCC, Clang, or any C89-compliant compiler.
* **Make**: Recommended for building the project source and generating the bundle.

### 1. Download the Single Header File

Download the latest **`qcs.h`** from the GitHub releases page:
```bash
curl -LJO https://github.com/SegusFaultise/qcs/releases/latest/download/qcs.h
```

Alternatively, you can build from source:
```bash
git clone https://github.com/SegusFaultise/qcs.git
cd qcs
python scripts/bundle.py
cp qcs.h /path/to/your/project/
```

### 2. Choose Your Parallelization Mode

Select the optimal parallelization mode for your hardware:

```c
/* GPU Mode */
#define QCS_GPU_OPENCL
#define QCS_IMPLEMENTATION
#include "qcs.h"

/* Or SIMD Mode  */
#define QCS_SIMD_ONLY
#define QCS_IMPLEMENTATION
#include "qcs.h"

/* Or Sequential Mode  */
#define QCS_IMPLEMENTATION
#include "qcs.h"
```

### 3. Basic Usage Example

In **exactly one** of your source files (e.g., `main.c`), include the implementation macro before including the header:

```c
/* example main.c - GPU accelerated version */
#define QCS_GPU_OPENCL
#define QCS_IMPLEMENTATION
#include "qcs.h"

int main() {
  /* Create a 3-qubit quantum circuit */
  t_q_circuit *qc = qc_create(3);

  /* Apply quantum gates */
  qc_h(qc, 0);           /* Apply Hadamard gate to qubit 0 */
  qc_cnot(qc, 0, 1);     /* Apply CNOT gate (control: 0, target: 1) */
  qc_x(qc, 2);           /* Apply Pauli-X gate to qubit 2 */

  /* Display results */
  qc_print_circuit(qc);
  qc_print_state(qc);

  /* Run Grover's search algorithm */
  t_q_circuit *grover_circuit = qc_create(3);
  qc_grover_search(grover_circuit, 5);  /* Search for state 5 */
  qc_print_circuit(grover_circuit);
  qc_print_state(grover_circuit);

  /* Cleanup */
  qc_destroy(qc);
  qc_destroy(grover_circuit);

  return 0;
}
```

### Compilation Examples

```bash
# GPU Mode 
gcc -O3 -march=native -DQCS_GPU_OPENCL -DQCS_IMPLEMENTATION main.c -lm -lOpenCL -o main

# SIMD Mode 
gcc -O3 -march=native -mavx2 -mfma -DQCS_SIMD_ONLY -DQCS_IMPLEMENTATION main.c -lm -o main

# Sequential Mode 
gcc -O3 -march=native -DQCS_IMPLEMENTATION main.c -lm -o main
```

---

## Performance Results

### Benchmark Results (20 qubits, Deutsch-Jozsa algorithm)

| Mode | Time | Speedup | Best For |
|------|------|---------|----------|
| **GPU OpenCL** | **0.242s** | **1.5x** | Very large problems, GPU systems |
| **Sequential** | 0.375s | 1.0x | Small problems, compatibility |
| **SIMD** | ~0.30s | 1.2x | CPU-bound problems, AVX2 |
| **OpenMP** | ~0.40s | 0.9x | Large problems, OpenMP systems |
| **Multi-threaded** | 0.959s | 0.39x | Medium problems, multi-core |

### GPU Acceleration Features

- **Persistent GPU Memory**: Reduces transfer overhead between gates
- **Optimized OpenCL Kernels**: Dedicated kernels for quantum gate operations
- **Automatic Device Detection**: Finds GPU across all OpenCL platforms
- **Optimized Work Groups**: 256-thread work groups for maximum GPU utilization

---

## Available Functions

### Circuit Management
- `qc_create()`, `qc_destroy()`, `qc_run()`, `qc_run_shots()`

### Quantum Gates
- **Basic Gates**: `qc_h()`, `qc_x()`, `qc_y()`, `qc_z()`, `qc_cnot()`
- **Rotation Gates**: `qc_rx()`, `qc_ry()`, `qc_rz()`, `qc_phase()`, `qc_cphase()`

### Measurement & Analysis
- `qc_measure()`, `qc_measure_all()`, `qc_get_probability()`, `qc_find_most_likely_state()`

### Algorithms
- `qc_grover_search()`, `qc_bernstein_vazirani()`, `qc_quantum_fourier_transform()`

### Utilities
- `qc_print_circuit()`, `qc_print_state()`, `qc_optimize()`, `qc_barrier()`

**All functions include complete JSDoc-style documentation with parameter descriptions and return values!**

---

## Examples

Check out the [`examples/`](examples/) directory for comprehensive examples demonstrating all parallelization modes:

- **`sequential_example.c`**: Sequential mode (baseline)
- **`multithread_example.c`**: Multi-threading with pthread
- **`openmp_example.c`**: OpenMP parallelization
- **`simd_example.c`**: SIMD (AVX2) optimization
- **`gpu_example.c`**: GPU OpenCL acceleration (fastest)

Run all examples:
```bash
cd examples
make test
```

---

## Documentation

* **[Examples README](examples/README.md)**: Comprehensive examples and performance comparison
* **[API Reference](API_REFERENCE.md)**: All functions and structs with complete documentation
* **[Contribution Guide](CONTRIBUTING.md)**: Information for developers and building the bundle

---

## Building from Source

```bash
git clone https://github.com/SegusFaultise/qcs.git
cd qcs

# Generate the single header file
python scripts/bundle.py

# Build and test
make test

# Run examples
cd examples
make test
```

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## Contributing

Contributions are welcome! Please read our [Contributing Guide](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.

---

## Acknowledgments

- Quantum computing community for algorithm implementations
- OpenCL developers for GPU acceleration support
- C89 standard maintainers for maximum compatibility
