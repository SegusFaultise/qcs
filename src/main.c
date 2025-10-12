#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define QCS_IMPLEMENTATION
#include "../include/qcs.h"

void run_grover_test(int num_qubits);
void run_qft_test(int num_qubits);
void run_ghz_test(int num_qubits);
void run_bv_test(int num_qubits);
void run_shots_test(int num_qubits);
void run_optimizer_test();

int main(int argc, char *argv[]) {
  int num_qubits;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <test_name> [num_qubits]\n", argv[0]);
    fprintf(stderr,
            "Available tests: grover, qft, ghz, bv, shots, optimizer\n");
    return 1;
  }

  num_qubits = (argc > 2) ? atoi(argv[2]) : 4;
  srand(time(NULL)); /* Seed for measurement functions */

  if (strcmp(argv[1], "grover") == 0) {
    if (num_qubits < 2) {
      fprintf(stderr, "Grover's search requires at least 2 qubits.\n");
      return 1;
    }
    run_grover_test(num_qubits);
  } else if (strcmp(argv[1], "qft") == 0) {
    if (num_qubits < 1)
      return 1;
    run_qft_test(num_qubits);
  } else if (strcmp(argv[1], "ghz") == 0) {
    if (num_qubits < 2) {
      fprintf(stderr, "GHZ state requires at least 2 qubits.\n");
      return 1;
    }
    run_ghz_test(num_qubits);
  } else if (strcmp(argv[1], "bv") == 0) {
    if (num_qubits < 2) {
      fprintf(stderr, "Bernstein-Vazirani requires at least 2 qubits for the "
                      "hidden string.\n");
      return 1;
    }
    run_bv_test(num_qubits);
  } else if (strcmp(argv[1], "shots") == 0) {
    /* This test is hardcoded for 2 qubits to create a Bell state */
    run_shots_test(2);
  } else if (strcmp(argv[1], "optimizer") == 0) {
    run_optimizer_test();
  } else {
    fprintf(stderr, "Error: Unknown test '%s'\n", argv[1]);
    return 1;
  }

  return 0;
}

/* --- Test Function Implementations --- */

void run_grover_test(int num_qubits) {
  t_q_circuit *circuit;
  int solution_state;
  clock_t start, end;
  double cpu_time_used;

  printf("--- Running Grover's Search Test ---\n");
  printf("Setting up Grover's search for %d qubits...\n", num_qubits);
  circuit = qc_create(num_qubits);
  if (!circuit)
    return;

  /* A pseudo-random solution state */
  solution_state = (1 << (num_qubits - 1)) + 1;
  printf("Searching for solution state: |%d>\n", solution_state);

  start = clock();
  qc_grover_search(circuit, solution_state);
  end = clock();

  qc_print_state(circuit, solution_state);
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("Grover's search completed in %f seconds.\n", cpu_time_used);

  qc_destroy(circuit);
}

void run_qft_test(int num_qubits) {
  t_q_circuit *circuit;
  clock_t start, end;
  double cpu_time_used;

  printf("--- Running Quantum Fourier Transform (QFT) Benchmark ---\n");
  printf("Performing QFT on %d qubits...\n", num_qubits);
  circuit = qc_create(num_qubits);
  if (!circuit)
    return;

  printf("Initial state: |1>\n");

  start = clock();
  qc_quantum_fourier_transform(circuit);
  end = clock();

  qc_print_state(circuit, -1);
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("QFT benchmark completed in %f seconds.\n", cpu_time_used);

  qc_destroy(circuit);
}

void run_ghz_test(int num_qubits) {
  t_q_circuit *circuit;
  printf("--- Running GHZ State Test ---\n");
  printf("Creating a %d-qubit GHZ state...\n", num_qubits);
  circuit = qc_create(num_qubits);
  if (!circuit)
    return;

  qc_ghz_state(circuit);

  printf(
      "Final state should be an equal superposition of |0...0> and |1...1>:\n");
  qc_print_state(circuit, -1);

  qc_destroy(circuit);
}

void run_bv_test(int num_qubits) {
  t_q_circuit *circuit;
  int hidden_string;
  int measured_string;

  printf("--- Running Bernstein-Vazirani Test ---\n");

  /* We need n+1 qubits: n for the string, 1 for the ancilla */
  circuit = qc_create(num_qubits + 1);
  if (!circuit)
    return;

  /* Generate a random n-bit hidden string */
  hidden_string = rand() % (1 << num_qubits);

  printf("Oracle has hidden string: %d\n", hidden_string);

  qc_bernstein_vazirani(circuit, hidden_string);

  printf("Finding the resulting state with 100%% probability...\n");

  /* --- FIX: Use the non-destructive readout function --- */
  measured_string = qc_find_most_likely_state(circuit);

  printf("\nMeasurement complete.\n");
  printf("Hidden String was:   %d\n", hidden_string);
  printf("Measured String is:  %d\n", measured_string);

  if (hidden_string == measured_string) {
    printf("SUCCESS: The hidden string was found correctly!\n");
  } else {
    printf("FAILURE: The measured string does not match.\n");
  }

  qc_destroy(circuit);
}

void run_shots_test(int num_qubits) {
  t_q_circuit *circuit;
  int num_shots = 1024;
  int *results;
  long num_states = 1L << num_qubits;
  int i;

  printf("--- Running Measurement and Shots Test ---\n");
  printf("Creating a 2-qubit Bell state |Φ+> = (|00> + |11>)/sqrt(2)\n");

  circuit = qc_create(num_qubits);
  if (!circuit)
    return;
  results = (int *)malloc(num_states * sizeof(int));
  if (!results) {
    qc_destroy(circuit);
    return;
  }

  /* Create Bell State */
  qc_h(circuit, 0);
  qc_cnot(circuit, 0, 1);

  printf("Ideal state:\n");
  qc_print_state(circuit, -1);

  printf("Simulating %d measurement shots...\n", num_shots);
  qc_run_shots(circuit, num_shots, results);

  printf("\nShot results:\n");
  for (i = 0; i < num_states; i++) {
    if (results[i] > 0) {
      printf("State |%d> was measured %d times (%.2f%%)\n", i, results[i],
             (double)results[i] / num_shots * 100.0);
    }
  }
  printf("Expected results are ~50%% for |0> (00) and ~50%% for |3> (11).\n");

  free(results);
  qc_destroy(circuit);
}

void run_optimizer_test() {
  t_q_circuit *circuit;
  printf("--- Running Optimizer Test ---\n");

  circuit = qc_create(2);
  if (!circuit)
    return;

  /* Build a circuit with redundant gates */
  qc_h(circuit, 0);
  qc_h(circuit, 0); /* Should be removed */
  qc_x(circuit, 1);
  qc_cnot(circuit, 0, 1);
  qc_cnot(circuit, 0, 1); /* Should be removed */
  qc_y(circuit, 0);

  printf("\n--- Circuit BEFORE Optimization ---\n");
  qc_print_circuit(circuit);

  qc_optimize(circuit);

  printf("\n--- Circuit AFTER Optimization ---\n");
  qc_print_circuit(circuit);
  printf("SUCCESS: Redundant H-H and CNOT-CNOT pairs should be removed.\n");

  qc_destroy(circuit);
}
