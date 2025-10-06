#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

struct t_q_matrix *q_gate_I(void) {
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

  CNOT->data[10] = c_one();
  CNOT->data[15] = c_one();

  return CNOT;
}

struct t_q_matrix *q_gate_oracle(int num_qubits, int solution_index) {
  int size = 1 << num_qubits;
  struct t_q_matrix *oracle;
  long i;

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

void old_q_apply_phase_flip(struct t_q_state *state, int target_index) {
  if (target_index >= 0 && target_index < state->size) {
    state->vector[target_index].number_real *= -1.0;
    state->vector[target_index].number_imaginary *= -1.0;
  } else {
    fprintf(stderr, "Error: Invalid target index for phase flip.\n");
  }
}

struct t_q_matrix *q_gate_diffusion(int num_qubits) {
  int size = 1 << num_qubits;
  struct t_q_matrix *diffusion;
  long i, j;
  double factor = 2.0 / size;

  diffusion = q_matrix_init(size, size);
  if (!diffusion)
    return NULL;

  for (i = 0; i < size; i++) {
    for (j = 0; j < size; j++) {
      if (i == j) {
        diffusion->data[i * size + j] = c_from_real(factor - 1.0);
      } else {
        diffusion->data[i * size + j] = c_from_real(factor);
      }
    }
  }

  return diffusion;
}

void q_apply_diffusion(struct t_q_state *state) {
  struct t_complex *vector = state->vector;
  struct t_complex *scratch = state->scratch_vector;
  long size = state->size;
  long i;

  struct t_complex mean = c_zero();
  for (i = 0; i < size; i++) {
    mean = c_add(mean, vector[i]);
  }
  mean.number_real /= size;
  mean.number_imaginary /= size;

  for (i = 0; i < size; i++) {
    struct t_complex two_mean;
    two_mean.number_real = 2.0 * mean.number_real;
    two_mean.number_imaginary = 2.0 * mean.number_imaginary;

    scratch[i].number_real = two_mean.number_real - vector[i].number_real;
    scratch[i].number_imaginary =
        two_mean.number_imaginary - vector[i].number_imaginary;
  }

  struct t_complex *temp = state->vector;
  state->vector = scratch;
  state->scratch_vector = temp;
}

void q_apply_phase_flip(struct t_q_state *state, int index) {
  if (state == NULL || index < 0 || index >= state->size) {
    fprintf(stderr, "Error: Invalid state or index for phase flip.\n");
    return;
  }

  state->vector[index] = c_mul(state->vector[index], c_from_real(-1.0));
}

void q_apply_1q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int target_qubit) {
  struct t_complex *vector = state->vector;
  struct t_complex *new_vector = state->scratch_vector;
  long i;
  long size = state->size;
  long step = 1L << target_qubit;
  long block_size = 1L << (target_qubit + 1);

  if (state == NULL || gate == NULL || target_qubit < 0 ||
      target_qubit >= state->qubits_num) {
    fprintf(stderr, "Error: Invalid arguments for 1-qubit gate application.\n");
    return;
  }

  for (i = 0; i < size; i += block_size) {
    long j;
    for (j = i; j < i + step; j++) {
      long index0 = j;
      long index1 = j + step;

      struct t_complex v0 = vector[index0];
      struct t_complex v1 = vector[index1];

      new_vector[index0] =
          c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
      new_vector[index1] =
          c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
    }
  }

  struct t_complex *temp = state->vector;
  state->vector = new_vector;
  state->scratch_vector = temp;
}

void q_apply_2q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int control_qubit, int target_qubit) {
  struct t_complex *vector = state->vector;
  struct t_complex *new_vector = state->scratch_vector;

  long i;
  long size = state->size;
  long c_bit = 1L << control_qubit;
  long t_bit = 1L << target_qubit;

  if (state == NULL || gate == NULL || control_qubit < 0 || target_qubit < 0 ||
      control_qubit >= state->qubits_num || target_qubit >= state->qubits_num) {
    fprintf(stderr, "Error: Invalid control/target qubit indices.\n");
    return;
  }

  for (i = 0; i < size; i++) {
    new_vector[i] = vector[i];
  }

  for (i = 0; i < size; i++) {
    if ((i & c_bit) == 0 && (i & t_bit) == 0) {
      long i00 = i;
      long i01 = i | t_bit;
      long i10 = i | c_bit;
      long i11 = i | t_bit | c_bit;

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

  state->vector = new_vector;
  state->scratch_vector = vector;
}
