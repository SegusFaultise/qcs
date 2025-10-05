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

struct t_q_matrix *q_gate_CNOT(void) {
  struct t_q_matrix *CNOT = q_matrix_init(4, 4);
  if (!CNOT)
    return NULL;

  CNOT->data[0] = c_one();
  CNOT->data[5] = c_one();

  CNOT->data[11] = c_one();
  CNOT->data[14] = c_one();

  return CNOT;
}

struct t_q_matrix *q_gate_oracle(int num_qubits, int solution_index) {
  int size = 1 << num_qubits;
  struct t_q_matrix *oracle;
  int i;

  if (solution_index < 0 || solution_index >= size) {
    fprintf(stderr, "Error: Invalid Oracle solution index.\n");
    return NULL;
  }

  oracle = q_matrix_init(size, size);
  if (!oracle)
    return NULL;

  for (i = 0; i < size; i++) {
    oracle->data[i * size + i] = c_one();
  }

  oracle->data[solution_index * size + solution_index] = c_from_real(-1.0);

  return oracle;
}

struct t_q_matrix *q_gate_U0(int num_qubits) {
  return q_gate_oracle(num_qubits, 0);
}

void q_apply_phase_flip(struct t_q_state *state, int target_index) {
  if (target_index >= 0 && target_index < state->size) {
    state->vector[target_index].number_real *= -1.0;
    state->vector[target_index].number_imaginary *= -1.0;
  } else {
    fprintf(stderr, "Error: Invalid target index for phase flip.\n");
  }
}

void q_apply_1q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int target_qubit) {
  struct t_complex *vector = state->vector;
  struct t_complex *new_vector =
      state->scratch_vector; /* 👈 Use scratch buffer */
  struct t_complex *temp;
  int k;
  int i;

  if (gate->rows != 2 || gate->cols != 2) {
    fprintf(stderr, "Error: Gate must be 2x2 for q_apply_1q_gate.\n");
    return;
  }
  if (target_qubit < 0 || target_qubit >= state->qubits_num) {
    fprintf(stderr, "Error: Invalid target qubit index.\n");
    return;
  }

  k = 1 << target_qubit;

  /* NOTE: The index calculation is simplified in the modern implementation.
     We will keep your original structure but ensure the core logic is sound. */

  for (i = 0; i < state->size; i++) {
    if ((i & k) == 0) {
      int i0 = i;
      int i1 = i | k;

      struct t_complex amp0 = vector[i0];
      struct t_complex amp1 = vector[i1];

      new_vector[i0] =
          c_add(c_mul(gate->data[0], amp0), c_mul(gate->data[1], amp1));

      new_vector[i1] =
          c_add(c_mul(gate->data[2], amp0), c_mul(gate->data[3], amp1));
    }
  }

  temp = state->vector;
  state->vector = new_vector;
  state->scratch_vector = temp;
}

void q_apply_2q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int control_qubit, int target_qubit) {
  struct t_complex *vector = state->vector;
  struct t_complex *new_vector =
      state->scratch_vector; /* 👈 Use scratch buffer */
  struct t_complex *temp;
  int size = state->size;

  int c_bit = 1 << control_qubit;
  int t_bit = 1 << target_qubit;
  int i;

  if (gate->rows != 4 || gate->cols != 4) {
    fprintf(stderr, "Error: Gate must be 4x4 for q_apply_2q_gate.\n");
    return;
  }
  if (control_qubit == target_qubit || control_qubit < 0 || target_qubit < 0 ||
      control_qubit >= state->qubits_num || target_qubit >= state->qubits_num) {
    fprintf(stderr, "Error: Invalid control/target qubit indices.\n");
    return;
  }

  for (i = 0; i < size; i++) {
    new_vector[i] = vector[i];
  }

  for (i = 0; i < size; i++) {
    if ((i & c_bit) == 0 && (i & t_bit) == 0) {
      int i00 = i;
      int i01 = i | t_bit;
      int i10 = i | c_bit;
      int i11 = i | t_bit | c_bit;

      struct t_complex amp_00 = vector[i00];
      struct t_complex amp_01 = vector[i01];
      struct t_complex amp_10 = vector[i10];
      struct t_complex amp_11 = vector[i11];

      new_vector[i00] = c_add(
          c_add(c_mul(gate->data[0], amp_00), c_mul(gate->data[1], amp_01)),
          c_add(c_mul(gate->data[2], amp_10), c_mul(gate->data[3], amp_11)));

      new_vector[i01] = c_add(
          c_add(c_mul(gate->data[4], amp_00), c_mul(gate->data[5], amp_01)),
          c_add(c_mul(gate->data[6], amp_10), c_mul(gate->data[7], amp_11)));

      new_vector[i10] = c_add(
          c_add(c_mul(gate->data[8], amp_00), c_mul(gate->data[9], amp_01)),
          c_add(c_mul(gate->data[10], amp_10), c_mul(gate->data[11], amp_11)));

      new_vector[i11] = c_add(
          c_add(c_mul(gate->data[12], amp_00), c_mul(gate->data[13], amp_01)),
          c_add(c_mul(gate->data[14], amp_10), c_mul(gate->data[15], amp_11)));
    }
  }

  temp = state->vector;
  state->vector = new_vector;
  state->scratch_vector = temp;
}
