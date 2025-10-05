#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

struct t_q_matrix *q_gate_i(void) {
  struct t_q_matrix *i = q_matrix_init(2, 2);

  if (!i) {
    return NULL;
  }

  i->data[0] = c_one();
  i->data[3] = c_one();

  return i;
}

struct t_q_matrix *q_gate_X(void) {
  struct t_q_matrix *X = q_matrix_init(2, 2);

  if (!X) {
    return NULL;
  }

  X->data[0] = c_one();
  X->data[2] = c_one();

  return X;
}

struct t_q_matrix *q_gate_H(void) {
  double root2_inv = 1.0 / sqrt(2.0);
  struct t_complex factor = c_from_real(root2_inv);
  struct t_complex minus_one = c_from_real(-1.0);

  struct t_q_matrix *H = q_matrix_init(2, 2);

  if (!H) {
    return NULL;
  }

  H->data[0] = factor;
  H->data[1] = factor;
  H->data[2] = factor;
  H->data[3] = c_mul(factor, minus_one);

  return H;
}

void q_apply_1q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int target_qubit) {
  struct t_complex *vector = state->vector;
  struct t_complex *new_vector;
  int k; /* Index of the bit that separates the two states (2^target_qubit) */
  int i; /* Loop index, iterating over half the state vector size */

  if (gate->rows != 2 || gate->cols != 2) {
    fprintf(stderr, "Error: Gate must be 2x2 for q_apply_1q_gate.\n");
    return;
  }
  if (target_qubit < 0 || target_qubit >= state->qubits_num) {
    fprintf(stderr, "Error: Invalid target qubit index.\n");
    return;
  }

  new_vector =
      (struct t_complex *)malloc(state->size * sizeof(struct t_complex));

  if (new_vector == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for new_vector.\n");
    return;
  }

  k = 1 << target_qubit;

  for (i = 0; i < state->size / 2; i++) {
    int i0;
    int i1;

    i0 = (i & ~(k - 1)) | (i & (k - 1));

    int i_base = (i / k) * (k << 1) + (i % k);

    i0 = i_base;
    i1 = i_base | k;

    struct t_complex amp0 = vector[i0];
    struct t_complex amp1 = vector[i1];

    new_vector[i0] =
        c_add(c_mul(gate->data[0], amp0), c_mul(gate->data[1], amp1));

    new_vector[i1] =
        c_add(c_mul(gate->data[2], amp0), c_mul(gate->data[3], amp1));
  }

  free(state->vector);
  state->vector = new_vector;
}
