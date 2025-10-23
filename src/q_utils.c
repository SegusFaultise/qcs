#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

#ifdef QCS_MULTI_THREAD
extern thread_pool_t *pool;
#endif

/**
 * Worker function for parallel normalization sum calculation
 * @param arg Thread arguments containing state and range
 */
static void normalize_sum_worker(void *arg) {
  struct t_thread_args *args = (struct t_thread_args *)arg;
  args->reduction_result.sums.partial_real_sum = 0.0;
  long i;
  for (i = args->start; i < args->end; i++) {
    args->reduction_result.sums.partial_real_sum +=
        c_norm_sq(args->state->vector[i]);
  }
}

/**
 * Worker function for parallel normalization division
 * @param arg Thread arguments containing state and range
 */
static void normalize_divide_worker(void *arg) {
  struct t_thread_args *args = (struct t_thread_args *)arg;
  double inv_norm = args->mean.number_real;
  long i;
  for (i = args->start; i < args->end; i++) {
    args->state->vector[i].number_real *= inv_norm;
    args->state->vector[i].number_imaginary *= inv_norm;
  }
  free(args);
}

/**
 * Normalize quantum state vector to unit length
 * @param state Quantum state to normalize
 */
void q_state_normalize(struct t_q_state *state) {
  if (state == NULL || state->vector == NULL)
    return;

#ifdef QCS_MULTI_THREAD
  int i;
  long size;
  struct t_thread_args **reduction_args_list;
  double total_norm_sq = 0.0;

  size = state->size;
  reduction_args_list =
      malloc(pool->num_threads * sizeof(struct t_thread_args *));
  if (!reduction_args_list)
    exit(EXIT_FAILURE);

  for (i = 0; i < pool->num_threads; i++) {
    long start, end;
    get_thread_work_range(size, pool->num_threads, i, &start, &end);

    reduction_args_list[i] = malloc(sizeof(struct t_thread_args));
    if (!reduction_args_list[i])
      exit(EXIT_FAILURE);

    reduction_args_list[i]->start = start;
    reduction_args_list[i]->end = end;
    reduction_args_list[i]->state = state;

    thread_pool_add_task(pool, normalize_sum_worker, reduction_args_list[i]);
  }
  thread_pool_wait(pool);

  for (i = 0; i < pool->num_threads; i++) {
    total_norm_sq +=
        reduction_args_list[i]->reduction_result.sums.partial_real_sum;
    free(reduction_args_list[i]);
  }
  free(reduction_args_list);

  if (total_norm_sq > 1e-12 && total_norm_sq != 1.0) {
    double inv_norm = 1.0 / sqrt(total_norm_sq);
    for (i = 0; i < pool->num_threads; i++) {
      long start, end;
      get_thread_work_range(size, pool->num_threads, i, &start, &end);

      struct t_thread_args *div_args = malloc(sizeof(struct t_thread_args));
      if (div_args == NULL) {
        exit(EXIT_FAILURE);
      }
      div_args->start = start;
      div_args->end = end;
      div_args->state = state;
      div_args->mean.number_real = inv_norm;

      thread_pool_add_task(pool, normalize_divide_worker, div_args);
    }
    thread_pool_wait(pool);
  }
#else
  long i;
  long size = state->size;
  double total_norm_sq = 0.0;

  for (i = 0; i < size; i++) {
    total_norm_sq += c_norm_sq(state->vector[i]);
  }

  if (total_norm_sq > 1e-12 && total_norm_sq != 1.0) {
    double inv_norm = 1.0 / sqrt(total_norm_sq);
    for (i = 0; i < size; i++) {
      state->vector[i].number_real *= inv_norm;
      state->vector[i].number_imaginary *= inv_norm;
    }
  }
#endif
}

/**
 * Calculate optimal number of Grover iterations
 * @param num_qubits Number of qubits in the system
 * @return Number of iterations needed
 */
int q_grover_iterations(int num_qubits) {
  double const m_pi = (3.14159265358979323846);
  double N = (double)(1 << num_qubits);
  double R;

  R = (m_pi / 4.0) * sqrt(N);

  return (int)floor(R);
}
