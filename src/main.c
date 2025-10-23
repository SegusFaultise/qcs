#include <stdio.h>
#include <time.h>
#include "../include/qcs.h"

int main(int argc, char *argv[]) {
  (void)argc;  /* Suppress unused parameter warning */
  (void)argv;  /* Suppress unused parameter warning */
    int num_qubits = 3;
    t_q_circuit *circuit;
    clock_t start, end;
    double cpu_time_used;
    int i, n;

    circuit = qc_create(num_qubits);
    if (!circuit) {
        fprintf(stderr, "Error: Failed to create quantum circuit.\n");
        return 1;
    }

    start = clock();
    
    /* Deutsch-Jozsa Algorithm */
    n = num_qubits - 1;  /* n input qubits, 1 ancilla qubit */
    
    /* Step 1: Initialize ancilla qubit to |1⟩ */
    qc_x(circuit, n);
    
    /* Step 2: Apply Hadamard gates to all qubits */
    for (i = 0; i < num_qubits; i++) {
        qc_h(circuit, i);
    }
    
    /* Step 3: Apply oracle function (simplified version) */
    for (i = 0; i < n; i++) {
        qc_cnot(circuit, i, n);
    }
    
    /* Step 4: Apply Hadamard gates to input qubits only */
    for (i = 0; i < n; i++) {
        qc_h(circuit, i);
    }
    
    /* Step 5: Measure input qubits */
    for (i = 0; i < n; i++) {
        qc_measure(circuit, i);
    }
    
    end = clock();
    
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Deutsch-Jozsa algorithm completed in %.6f seconds\n", cpu_time_used);
   
    qc_destroy(circuit);

  return 0;
}

