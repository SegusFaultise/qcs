#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

int main() {
  struct t_q_state *state = NULL;
  struct t_q_matrix *H = NULL;
  int num_qubits = 1;

  printf("--- Quantum Computing Simulator (C89) Test ---\n");

  state = q_state_init(num_qubits);
  if (state == NULL) {
    return 1;
  }

  q_state_set_basis(state, 0);
  printf("\n[Step 1] Initial state: |0>\n");
  q_state_print(state);

  H = q_gate_H();
  if (H == NULL) {
    q_state_free(state);
    return 1;
  }
  printf("\n[Step 2] Gate to apply: Hadamard (H)\n");
  q_matrix_print(H);

  q_apply_1q_gate(state, H, 0);

  printf("\n[Step 3] State after H gate: |+>\n");
  q_state_normalize(state);
  q_state_print(state);

  /* Expected Result:
   * |0>: approx 0.7071 + i0.0000
   * |1>: approx 0.7071 + i0.0000
   * (where 0.7071 is 1/sqrt(2))
   */

  q_matrix_free(H);
  q_state_free(state);

  printf("\n--- Test Complete ---\n");
  return 0;
}
