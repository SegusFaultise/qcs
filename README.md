<div align="center">

  <h1>QCS: Quantum Circuit Simulator</h1>

  <p>
    <strong>A fast, C89-compliant single-header library for quantum state vector simulation.</strong>
    <br />
    QCS provides core quantum logic gates and fundamental algorithms (like Grover's Search) ready for high-performance C applications.
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

QCS (Quantum Circuit Simulator) is a lightweight, high-performance library designed for classical simulation of quantum circuits. Unlike larger frameworks, QCS is engineered for maximum portability and minimal dependencies, targeting the **C89 standard** to ensure compatibility with nearly any C compiler and system architecture.

The library uses the **state vector simulation** model, storing the full quantum state in memory as a complex-valued vector. Its primary focus is providing a foundation for small to medium-sized quantum algorithms within existing C codebases.

### Single-Header Distribution

The library is distributed as a single file, **`qcs.h`**, making integration into your project as simple as dropping the file into your source tree and adding two lines of code. No complex build systems or installation steps required for users.

---

## Key Features

* **Single-File Integration**: Delivered entirely in one header file (`qcs.h`) for zero-dependency integration.
* **C89 / ANSI C Compliant**: Designed for maximum compatibility with legacy systems.
* **Core Gate Set**: Includes fundamental quantum gates (Hadamard, Pauli-X, CNOT) and complex number operations.
* **Grover's Search Algorithm**: Built-in support for running Grover's quantum search algorithm efficiently.
* **State Vector Simulation**: Simulates the full quantum state, allowing for precise probability measurement and state inspection.

---

## Getting Started

### Prerequisites

You only need a standard C compiler and the ability to link against the math library (`-lm`).

* **C Compiler**: GCC, Clang, or any C89-compliant compiler.
* **Make**: Recommended for building the project source and generating the bundle.

### 1. Integrate the Single Header File

Download the latest **`qcs.h`** from the [Releases page] and place it in your project directory.

In **exactly one** of your source files (e.g., `main.c`), include the implementation macro before including the header:

```c
/* example main.c */
#define QCS_IMPLEMENTATION
#include "qcs_single.h"

int main() {
  t_q_circuit *qc = qc_create(3);

  qc_h(qc, 0);
  qc_cnot(qc, 0, 1);
  qc_x(qc, 2);

  qc_print_circuit(qc);
  qc_print_state(qc);

  t_q_circuit *grover_circuit = qc_create(3);
  qc_grover_search(grover_circuit, 5, 10);
  qc_print_circuit(grover_circuit);
  qc_print_state(grover_circuit);

  qc_destroy(qc);
  qc_destroy(grover_circuit);

  return 0;
}
    ```
