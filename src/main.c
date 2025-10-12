#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define QCS_IMPLEMENTATION
#include "../include/qcs.h"

int main(int argc, char *argv[]) {
  int num_qubits;
  int solution_state;
  t_q_circuit *grover_circuit;
  clock_t start, end;
  double cpu_time_used;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <num_qubits>\n", argv[0]);
    return 1;
  }
  num_qubits = atoi(argv[1]);

  printf("Setting up Grover's search for %d qubits...\n", num_qubits);
  grover_circuit = qc_create(num_qubits);
  if (!grover_circuit) {
    fprintf(stderr, "Error: Failed to create circuit. Not enough memory?\n");
    return 1;
  }
  solution_state = (1 << (num_qubits - 1)) + 1;

  start = clock();
  qc_grover_search(grover_circuit, solution_state);
  end = clock();

  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("----------------------------------------\n");
  printf("Grover's search on %d qubits completed.\n", num_qubits);
  printf("Execution time: %f seconds\n", cpu_time_used);
  printf("----------------------------------------\n");

  qc_destroy(grover_circuit);

  return 0;
}
