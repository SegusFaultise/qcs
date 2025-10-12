#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

#define CACHE_LINE_SIZE 64

extern thread_pool_t *pool;

static void q_state_init_worker(void *arg) {
  struct t_thread_args *args = (struct t_thread_args *)arg;
  long i;

  for (i = args->start; i < args->end; i++) {
    args->state->vector[i] = c_zero();
    args->state->scratch_vector[i] = c_zero();
  }
  free(args);
}

struct t_q_state *q_state_init(int num_qubits) {
  struct t_q_state *state;
  long size;
  int i;
  int ret;

  if (num_qubits <= 0) {
    fprintf(stderr, "Error: Number of qubits must be positive.\n");
    return NULL;
  }

  size = 1L << num_qubits;
  state = (struct t_q_state *)malloc(sizeof(struct t_q_state));
  if (state == NULL) {
    return NULL;
  }

  ret = posix_memalign((void **)&state->vector, CACHE_LINE_SIZE,
                       size * sizeof(struct t_complex));
  if (ret != 0) {
    fprintf(stderr,
            "Error: Aligned memory allocation failed for state vector.\n");
    free(state);
    return NULL;
  }

  ret = posix_memalign((void **)&state->scratch_vector, CACHE_LINE_SIZE,
                       size * sizeof(struct t_complex));
  if (ret != 0) {
    fprintf(stderr,
            "Error: Aligned memory allocation failed for scratch vector.\n");
    free(state->vector);
    free(state);
    return NULL;
  }

  state->qubits_num = num_qubits;
  state->size = size;

  for (i = 0; i < pool->num_threads; i++) {
    long start, end;

    get_thread_work_range(size, pool->num_threads, i, &start, &end);

    struct t_thread_args *args = malloc(sizeof(struct t_thread_args));
    if (args == NULL) {
      fprintf(stderr,
              "Error: Failed to allocate memory for thread arguments.\n");
      exit(EXIT_FAILURE);
    }

    args->start = start;
    args->end = end;
    args->state = state;

    thread_pool_add_task(pool, q_state_init_worker, args);
  }

  thread_pool_wait(pool);
  state->vector[0] = c_one();
  return state;
}

void q_state_free(struct t_q_state *state) {
  if (state) {
    if (state->vector)
      free(state->vector);
    if (state->scratch_vector)
      free(state->scratch_vector);
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

  printf("--- Quantum State (%d Qubits) ---\n", state->qubits_num);

  for (i = 0; i < max_print; i++) {
    printf("|%ld>: %f + i%f%s\n", i, state->vector[i].number_real,
           state->vector[i].number_imaginary,
           i == solution_index ? " <-- SOLUTION" : "");
  }

  if (solution_index >= max_print && solution_index < state->size - 1) {
    printf("...\n");
    printf("|%d>: %f + i%f <-- SOLUTION\n", solution_index,
           state->vector[solution_index].number_real,
           state->vector[solution_index].number_imaginary);
  }

  if (state->size > max_print) {
    printf("...\n");
    long last_index = state->size - 1;
    printf("|%ld>: %f + i%f%s\n", last_index,
           state->vector[last_index].number_real,
           state->vector[last_index].number_imaginary,
           last_index == solution_index ? " <-- SOLUTION" : "");
  }

  printf("----------------------------------\n");
}
