#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

extern thread_pool_t *pool;

static void q_apply_1q_gate_worker(void *arg) {
  struct t_thread_args *args = (struct t_thread_args *)arg;
  struct t_q_state *state = args->state;
  const struct t_q_matrix *gate = args->gate;
  int target_qubit = args->target_qubit;
  long step = 1L << target_qubit;
  long block_size = 1L << (target_qubit + 1);
  long i;

  for (i = args->start; i < args->end; i += block_size) {
    long j;
    for (j = i; j < i + step; j++) {
      long index0 = j;
      long index1 = j + step;
      struct t_complex v0 = state->vector[index0];
      struct t_complex v1 = state->vector[index1];

      state->scratch_vector[index0] =
          c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
      state->scratch_vector[index1] =
          c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
    }
  }
  free(args);
}

static void diffusion_mean_worker(void *arg) {
  struct t_thread_args *args = (struct t_thread_args *)arg;
  args->reduction_result.sums.partial_real_sum = 0.0;
  args->reduction_result.sums.partial_imag_sum = 0.0;
  long i;
  for (i = args->start; i < args->end; i++) {
    args->reduction_result.sums.partial_real_sum +=
        args->state->vector[i].number_real;
    args->reduction_result.sums.partial_imag_sum +=
        args->state->vector[i].number_imaginary;
  }
}

static void diffusion_transform_worker(void *arg) {
  struct t_thread_args *args = (struct t_thread_args *)arg;
  struct t_complex mean = args->mean;
  struct t_complex two_mean = {2.0 * mean.number_real,
                               2.0 * mean.number_imaginary};
  long i;
  for (i = args->start; i < args->end; i++) {
    args->state->scratch_vector[i].number_real =
        two_mean.number_real - args->state->vector[i].number_real;
    args->state->scratch_vector[i].number_imaginary =
        two_mean.number_imaginary - args->state->vector[i].number_imaginary;
  }
  free(args);
}

void q_apply_1q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int target_qubit) {
  long size, block_size, num_blocks;
  int i;

  if (state == NULL || gate == NULL || target_qubit < 0 ||
      target_qubit >= state->qubits_num) {
    fprintf(stderr, "Error: Invalid arguments for 1-qubit gate application.\n");
    return;
  }

  memcpy(state->scratch_vector, state->vector,
         state->size * sizeof(struct t_complex));

  size = state->size;
  block_size = 1L << (target_qubit + 1);
  num_blocks = size / block_size;

  for (i = 0; i < pool->num_threads; i++) {
    long start_block, end_block;
    get_thread_work_range(num_blocks, pool->num_threads, i, &start_block,
                          &end_block);

    struct t_thread_args *args = malloc(sizeof(struct t_thread_args));
    if (!args)
      exit(EXIT_FAILURE);

    args->start = start_block * block_size;
    args->end = end_block * block_size;
    args->state = state;
    args->gate = gate;
    args->target_qubit = target_qubit;

    thread_pool_add_task(pool, q_apply_1q_gate_worker, args);
  }
  thread_pool_wait(pool);

  struct t_complex *temp = state->vector;
  state->vector = state->scratch_vector;
  state->scratch_vector = temp;
}

void q_apply_phase_flip(struct t_q_state *state, int index) {
  if (state == NULL || index < 0 || index >= state->size) {
    fprintf(stderr, "Error: Invalid state or index for phase flip.\n");
    return;
  }
  memcpy(state->scratch_vector, state->vector,
         state->size * sizeof(struct t_complex));
  state->scratch_vector[index] =
      c_mul(state->scratch_vector[index], c_from_real(-1.0));
  struct t_complex *temp = state->vector;
  state->vector = state->scratch_vector;
  state->scratch_vector = temp;
}

void q_apply_diffusion(struct t_q_state *state) {
  long size = state->size;
  int i;
  struct t_thread_args **reduction_args_list;
  double total_real = 0.0, total_imag = 0.0;
  struct t_complex mean;

  reduction_args_list =
      malloc(pool->num_threads * sizeof(struct t_thread_args *));
  if (!reduction_args_list)
    exit(EXIT_FAILURE);

  /* Calculate mean in parallel */
  for (i = 0; i < pool->num_threads; i++) {
    long start, end;
    get_thread_work_range(size, pool->num_threads, i, &start, &end);

    reduction_args_list[i] = malloc(sizeof(struct t_thread_args));
    if (!reduction_args_list[i])
      exit(EXIT_FAILURE);

    reduction_args_list[i]->start = start;
    reduction_args_list[i]->end = end;
    reduction_args_list[i]->state = state;
    thread_pool_add_task(pool, diffusion_mean_worker, reduction_args_list[i]);
  }
  thread_pool_wait(pool);

  /* Aggregate results */
  for (i = 0; i < pool->num_threads; i++) {
    total_real +=
        reduction_args_list[i]->reduction_result.sums.partial_real_sum;
    total_imag +=
        reduction_args_list[i]->reduction_result.sums.partial_imag_sum;
    free(reduction_args_list[i]);
  }
  free(reduction_args_list);

  mean.number_real = total_real / size;
  mean.number_imaginary = total_imag / size;

  /* Apply transformation in parallel */
  for (i = 0; i < pool->num_threads; i++) {
    long start, end;
    get_thread_work_range(size, pool->num_threads, i, &start, &end);

    struct t_thread_args *transform_args = malloc(sizeof(struct t_thread_args));
    if (!transform_args)
      exit(EXIT_FAILURE);

    transform_args->start = start;
    transform_args->end = end;
    transform_args->state = state;
    transform_args->mean = mean;
    thread_pool_add_task(pool, diffusion_transform_worker, transform_args);
  }
  thread_pool_wait(pool);

  /* Commit the new state */
  struct t_complex *temp = state->vector;
  state->vector = state->scratch_vector;
  state->scratch_vector = temp;
}

static void q_apply_2q_gate_worker(void *arg) {
  struct t_thread_args *args = (struct t_thread_args *)arg;
  struct t_q_state *state = args->state;
  const struct t_q_matrix *gate = args->gate;
  long i;

  long c_bit = 1L << args->control_qubit;
  long t_bit = 1L << args->target_qubit;

  for (i = args->start; i < args->end; i++) {
    if ((i & c_bit) != 0 && (i & t_bit) == 0) {
      long index0 = i;
      long index1 = i | t_bit;

      struct t_complex v0 = state->vector[index0];
      struct t_complex v1 = state->vector[index1];

      state->scratch_vector[index0] =
          c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
      state->scratch_vector[index1] =
          c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
    }
  }
  free(args);
}

void q_apply_2q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int control_qubit, int target_qubit) {
  long size;
  int i;

  if (state == NULL || gate == NULL || control_qubit < 0 || target_qubit < 0 ||
      control_qubit >= state->qubits_num || target_qubit >= state->qubits_num ||
      control_qubit == target_qubit) {
    fprintf(stderr, "Error: Invalid arguments for 2-qubit gate application.\n");
    return;
  }

  memcpy(state->scratch_vector, state->vector,
         state->size * sizeof(struct t_complex));

  size = state->size;
  for (i = 0; i < pool->num_threads; i++) {
    long start, end;
    get_thread_work_range(size, pool->num_threads, i, &start, &end);

    struct t_thread_args *args = malloc(sizeof(struct t_thread_args));
    if (!args)
      exit(EXIT_FAILURE);

    args->start = start;
    args->end = end;
    args->state = state;
    args->gate = gate;
    args->control_qubit = control_qubit;
    args->target_qubit = target_qubit;
    thread_pool_add_task(pool, q_apply_2q_gate_worker, args);
  }
  thread_pool_wait(pool);

  struct t_complex *temp = state->vector;
  state->vector = state->scratch_vector;
  state->scratch_vector = temp;
}

struct t_q_matrix *q_gate_I(void) {
  struct t_q_matrix *i = q_matrix_init(2, 2);
  if (!i)
    return NULL;
  i->data[0] = c_one();
  i->data[3] = c_one();
  return i;
}

struct t_q_matrix *q_gate_X(void) {
  struct t_q_matrix *X = q_matrix_init(2, 2);
  if (!X)
    return NULL;
  X->data[1] = c_one();
  X->data[2] = c_one();
  return X;
}

struct t_q_matrix *q_gate_H(void) {
  double root2_inv = 1.0 / sqrt(2.0);
  struct t_complex factor = c_from_real(root2_inv);
  struct t_q_matrix *H = q_matrix_init(2, 2);
  if (!H)
    return NULL;
  H->data[0] = factor;
  H->data[1] = factor;
  H->data[2] = factor;
  H->data[3] = c_from_real(-root2_inv);
  return H;
}

struct t_q_matrix *q_gate_CP(double angle) {
  struct t_q_matrix *CP = q_matrix_init(2, 2);
  if (!CP)
    return NULL;

  CP->data[0] = c_one();
  CP->data[3].number_real = cos(angle);
  CP->data[3].number_imaginary = sin(angle);
  return CP;
}

struct t_q_matrix *q_gate_Y(void) {
  struct t_q_matrix *Y = q_matrix_init(2, 2);
  if (!Y)
    return NULL;
  Y->data[1].number_imaginary = -1.0;
  Y->data[2].number_imaginary = 1.0;
  return Y;
}

struct t_q_matrix *q_gate_Z(void) {
  struct t_q_matrix *Z = q_matrix_init(2, 2);
  if (!Z)
    return NULL;
  Z->data[0] = c_one();
  Z->data[3] = c_from_real(-1.0);
  return Z;
}

struct t_q_matrix *q_gate_P(double angle) {
  struct t_q_matrix *P = q_matrix_init(2, 2);
  if (!P)
    return NULL;
  P->data[0] = c_one();
  P->data[3].number_real = cos(angle);
  P->data[3].number_imaginary = sin(angle);
  return P;
}

struct t_q_matrix *q_gate_RX(double angle) {
  struct t_q_matrix *RX = q_matrix_init(2, 2);
  double cos_half = cos(angle / 2.0);
  double sin_half = sin(angle / 2.0);
  if (!RX)
    return NULL;

  RX->data[0].number_real = cos_half;
  RX->data[1].number_imaginary = -sin_half;
  RX->data[2].number_imaginary = -sin_half;
  RX->data[3].number_real = cos_half;

  return RX;
}

struct t_q_matrix *q_gate_RY(double angle) {
  struct t_q_matrix *RY = q_matrix_init(2, 2);
  double cos_half = cos(angle / 2.0);
  double sin_half = sin(angle / 2.0);
  if (!RY)
    return NULL;

  RY->data[0].number_real = cos_half;
  RY->data[1].number_real = -sin_half;
  RY->data[2].number_real = sin_half;
  RY->data[3].number_real = cos_half;

  return RY;
}

struct t_q_matrix *q_gate_RZ(double angle) {
  struct t_q_matrix *RZ = q_matrix_init(2, 2);
  double cos_half = cos(angle / 2.0);
  double sin_half = sin(angle / 2.0);
  if (!RZ)
    return NULL;

  RZ->data[0].number_real = cos_half;
  RZ->data[0].number_imaginary = -sin_half;
  RZ->data[3].number_real = cos_half;
  RZ->data[3].number_imaginary = sin_half;

  return RZ;
}
