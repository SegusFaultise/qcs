#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

struct t_q_state *q_state_init(int num_qubits) {
  struct t_q_state *state;
  long size;

  if (num_qubits <= 0) {
    fprintf(stderr, "Error: Number of qubits must be positive.\n");
    return NULL;
  }

  size = 1L << num_qubits;

  state = (struct t_q_state *)malloc(sizeof(struct t_q_state));
  if (state == NULL) {
    return NULL;
  }

  state->vector = (struct t_complex *)calloc(size, sizeof(struct t_complex));
  if (state->vector == NULL) {
    /* Cleanup main struct on failure */
    free(state);
    return NULL;
  }

  state->scratch_vector =
      (struct t_complex *)calloc(size, sizeof(struct t_complex));
  if (state->scratch_vector == NULL) {
    fprintf(stderr, "Error: Could not allocate scratch vector.\n");
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
  long i;

  if (state == NULL || index_basis < 0 || index_basis >= state->size) {
    fprintf(stderr, "Error: Invalid state or basis index\n");
    return;
  }

  for (i = 0; i < state->size; i++) {
    state->vector[i] = c_zero();
  }

  state->vector[index_basis] = c_one();
}

void q_state_print(const struct t_q_state *state, int solution_index) {
  long i;
  int max_print = state->size > 8 ? 4 : state->size;
  int show_solution = 0;

  printf("--- Quantum State (%d Qubits) ---\n", state->qubits_num);

  for (i = 0; i < max_print; i++) {
    if (i == solution_index)
      show_solution = 1;
  }

  for (i = 0; i < max_print; i++) {
    printf("|%ld>: %f + i%f%s\n", i, state->vector[i].number_real,
           state->vector[i].number_imaginary,
           i == solution_index ? " <-- SOLUTION" : "");
  }

  if (!show_solution && solution_index >= 0 && solution_index < state->size) {
    printf("...\n");
    printf("|%d>: %f + i%f <-- SOLUTION\n", solution_index,
           state->vector[solution_index].number_real,
           state->vector[solution_index].number_imaginary);
    printf("...\n");
  } else if (state->size > 8) {
    printf("...\n");
  }

  if (state->size > 8) {
    printf("|%ld>: %f + i%f\n", state->size - 1,
           state->vector[state->size - 1].number_real,
           state->vector[state->size - 1].number_imaginary);
  }
  printf("----------------------------------\n");
}
