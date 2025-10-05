#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

int main() {
  struct t_q_state *state = NULL;
  struct t_q_matrix *H = NULL;
  struct t_q_matrix *Oracle = NULL;
  struct t_q_matrix *U0 = NULL;

  int num_qubits = 3;
  int solution_index = 6;
  int num_iterations;
  int i, q;

  printf("--- Grover's Algorithm (N=%d) Test ---\n", num_qubits);

  num_iterations = q_grover_iterations(num_qubits);
  printf("Search space size (N): %d. Optimal iterations (R): %d\n",
         (1 << num_qubits), num_iterations);

  state = q_state_init(num_qubits);
  if (state == NULL)
    return 1;

  q_state_set_basis(state, 0);
  H = q_gate_H();
  if (H == NULL) {
    q_state_free(state);
    return 1;
  }

  for (q = 0; q < num_qubits; q++) {
    q_apply_1q_gate(state, H, q);
  }
  printf("\n[Step 1] State after H^n (Uniform Superposition):\n");
  q_state_normalize(state);
  q_state_print(state);

  Oracle = q_gate_oracle(num_qubits, solution_index);
  U0 = q_gate_U0(num_qubits);
  if (Oracle == NULL || U0 == NULL) {
    return 1;
  }

  for (i = 0; i < num_iterations; i++) {
    printf("\n--- Iteration %d ---\n", i + 1);

    q_apply_phase_flip(state, solution_index);

    for (q = 0; q < num_qubits; q++) {
      q_apply_1q_gate(state, H, q);
    }

    q_apply_phase_flip(state, 0);

    for (q = 0; q < num_qubits; q++) {
      q_apply_1q_gate(state, H, q);
    }

    q_state_normalize(state);
  }

  printf("\n[Step 5] Final State after %d Iterations:\n", num_iterations);
  q_state_print(state);

  q_matrix_free(H);
  q_matrix_free(Oracle);
  q_matrix_free(U0);
  q_state_free(state);

  printf("\n--- Test Complete ---\n");
  return 0;
}
