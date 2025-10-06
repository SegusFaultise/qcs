#include <math.h>
#include <stdio.h>

#include "internal.h"

void q_state_normalize(struct t_q_state *state) {
  double total_norm_sq = 0.0;
  double inv_norm;
  long i;

  if (state == NULL || state->vector == NULL) {
    fprintf(stderr, "Error: Cannot normalize a NULL state.\n");
    return;
  }

  for (i = 0; i < state->size; i++) {
    total_norm_sq += c_norm_sq(state->vector[i]);
  }

  if (total_norm_sq == 1.0) {
    return;
  }
  if (total_norm_sq < 1e-12) {
    fprintf(stderr, "Warning: State vector has near-zero norm (uninitialized "
                    "or collapsed).\n");
    return;
  }

  inv_norm = 1.0 / sqrt(total_norm_sq);

  for (i = 0; i < state->size; i++) {
    state->vector[i].number_real *= inv_norm;
    state->vector[i].number_imaginary *= inv_norm;
  }
}

int q_grover_iterations(int num_qubits) {
  double const m_pi = (3.14159265358979323846);
  double N = (double)(1 << num_qubits);
  double R;

  R = (m_pi / 4.0) * sqrt(N);

  return (int)floor(R);
}
