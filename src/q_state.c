#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

struct t_q_state *q_state_init(int qubits_num) {
  struct t_q_state *state;
  int size;

  if (qubits_num <= 0) {
    fprintf(stderr, "Error: Number of qubits must be positive\n");
    return NULL;
    ;
  }

  size = 1 << qubits_num;

  state = (struct t_q_state *)malloc(sizeof(struct t_q_state));
  if (state == NULL) {
    fprintf(stderr, "Error: could not allocate t_q_state structure\n");
    return NULL;
  }

  state->vector = (struct t_complex *)calloc(size, sizeof(struct t_complex));
  if (state->vector == NULL) {
    fprintf(stderr, "Error: could not allocate state vector\n");
    return NULL;
  }

  state->qubits_num = qubits_num;
  state->size = size;

  return state;
}

void q_state_free(struct t_q_state *state) {
  if (state) {
    if (state->vector) {
      free(state->vector);
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
    printf("|%d>: %f + i%f\n", state->size - 1,
           state->vector[state->size - 1].number_real,
           state->vector[state->size - 1].number_imaginary);
  }
  printf("----------------------------------\n");
}
