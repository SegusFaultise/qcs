#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

struct t_q_state *q_state_init(int num_qubits) {
  struct t_q_state *state;
  long size;
  int ret;
  long i;

  if (num_qubits <= 0) {
    fprintf(stderr, "Error: Number of qubits must be positive.\n");
    return NULL;
  }

  size = 1L << num_qubits;

  state = (struct t_q_state *)malloc(sizeof(struct t_q_state));
  if (state == NULL) {
    return NULL;
  }

  ret = posix_memalign((void **)&state->vector, 64,
                       size * sizeof(struct t_complex));

  if (ret != 0) {
    fprintf(stderr,
            "Error: posix_memalign failed for state vector (Code %d).\n", ret);

    free(state);
    return NULL;
  }

  if (ret != 0) {
    fprintf(stderr,
            "Error: posix_memalign failed for state vector (Code %d).\n", ret);
    free(state);
    return NULL;
  }

  for (i = 0; i < size; i++) {
    state->vector[i] = c_zero();
  }

  ret = posix_memalign((void **)&state->scratch_vector, 64,
                       size * sizeof(struct t_complex));
  if (ret != 0) {
    fprintf(stderr,
            "Error: posix_memalign failed for scratch vector (Code %d).\n",
            ret);

    free(state->vector);
    free(state);
    return NULL;
  }

  state->qubits_num = num_qubits;
  state->size = size;

  return state;
}

void q_state_free(struct t_q_state *state) {
  if (state) {
    if (state->vector) {
      free(state->vector);
    }
    if (state->scratch_vector) {
      free(state->scratch_vector);
    }
    free(state);
  }
}

void q_state_set_basis(struct t_q_state *state, int index_basis) {
  int i;

  if (state == NULL || index_basis < 0 || index_basis >= state->size) {
    fprintf(stderr, "Error: Invalid state or basis index\n");
    return;
  }

  for (i = 0; i < state->size; i++) {
    state->vector[i] = c_zero();
  }

  state->vector[index_basis] = c_one();
}

void q_state_print(const struct t_q_state *state) {
  int i;
  int max_print = state->size > 8 ? 4 : state->size;

  printf("--- Quantum State (%d Qubits) ---\n", state->qubits_num);

  for (i = 0; i < max_print; i++) {
    printf("|%d>: %f + i%f\n", i, state->vector[i].number_real,
           state->vector[i].number_imaginary);
  }

  if (state->size > 8) {
    printf("...\n");
    printf("|%ld>: %f + i%f\n", state->size - 1,
           state->vector[state->size - 1].number_real,
           state->vector[state->size - 1].number_imaginary);
  }
  printf("----------------------------------\n");
}
