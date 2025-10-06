## Getting Started

### Prerequisites

You only need a standard C compiler and the ability to link against the math library (`-lm`).

* **C Compiler**: GCC, Clang, or any C89-compliant compiler.
* **Make**: Recommended for building the project source and generating the bundle.

### 1. Integrate the Single Header File

Download the latest **`qcs.h`** from the releases page and place it in your project directory.
```bash
curl -LJO https://github.com/SegusFaultise/qcs/releases/download/v1.0.0/qcs.h
```

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
