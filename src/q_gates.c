#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

#ifdef QCS_MULTI_THREAD
static void q_apply_1q_gate_worker(void *arg);
static void q_apply_2q_gate_worker(void *arg);
#endif

#ifdef QCS_GPU_OPENCL
void q_apply_1q_gate_gpu(struct t_q_state *state, const struct t_q_matrix *gate, int target_qubit);
void q_apply_2q_gate_gpu(struct t_q_state *state, const struct t_q_matrix *gate, 
                         int control_qubit, int target_qubit);
#endif

/**
 * Apply a 1-qubit quantum gate to the quantum state
 * @param state Quantum state vector
 * @param gate 2x2 gate matrix
 * @param target_qubit Target qubit index
 */
void q_apply_1q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int target_qubit) {
  long size = state->size;
  long step = 1L << target_qubit;
  long block_size = 1L << (target_qubit + 1);
  long i, j;

  if (state == NULL || gate == NULL || target_qubit < 0 ||
      target_qubit >= state->qubits_num) {
    fprintf(stderr, "Error: Invalid arguments for 1-qubit gate application.\n");
    return;
  }

  #ifdef QCS_GPU_OPENCL
    q_apply_1q_gate_gpu(state, gate, target_qubit);
  #elif defined(QCS_CPU_OPENMP)
    c_copy_simd(state->scratch_vector, state->vector, size);
    
    #ifdef _OPENMP
    #pragma omp parallel for
    for (i = 0; i < size; i += block_size) {
        for (j = i; j < i + step; j++) {
            long index0 = j;
            long index1 = j + step;
            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        }
    }
    #else
    for (i = 0; i < size; i += block_size) {
        for (j = i; j < i + step; j++) {
            long index0 = j;
            long index1 = j + step;
            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        }
    }
    #endif
    
  #elif defined(QCS_GPU_OPENCL)
    c_copy_gpu_real(state->scratch_vector, state->vector, size);
    
    for (i = 0; i < size; i += block_size) {
        for (j = i; j < i + step; j++) {
            long index0 = j;
            long index1 = j + step;
            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        }
    }
    
  #elif defined(QCS_SIMD_ONLY)
    c_copy_simd(state->scratch_vector, state->vector, size);
    
    for (i = 0; i < size; i += block_size) {
        for (j = i; j < i + step; j++) {
            long index0 = j;
            long index1 = j + step;
            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        }
    }
    
  #elif defined(QCS_MULTI_THREAD)
    extern thread_pool_t *pool;
    
    memcpy(state->scratch_vector, state->vector, size * sizeof(struct t_complex));

    int effective_threads = (pool->num_threads > 4) ? 4 : pool->num_threads;
    long num_blocks = size / block_size;
    long blocks_per_thread = (num_blocks + effective_threads - 1) / effective_threads;
    int k;

    for (k = 0; k < effective_threads; k++) {
        long start_block = k * blocks_per_thread;
        long end_block = (k + 1) * blocks_per_thread;
        if (end_block > num_blocks) end_block = num_blocks;
        
        if (start_block >= end_block) break;

        struct t_thread_args *args = malloc(sizeof(struct t_thread_args));
        if (!args) exit(EXIT_FAILURE);

        args->start = start_block * block_size;
        args->end = end_block * block_size;
        args->state = state;
        args->gate = gate;
        args->target_qubit = target_qubit;

        thread_pool_add_task(pool, q_apply_1q_gate_worker, args);
    }
    thread_pool_wait(pool);
    
  #else
    memcpy(state->scratch_vector, state->vector, size * sizeof(struct t_complex));

    for (i = 0; i < size; i += block_size) {
        for (j = i; j < i + step; j++) {
            long index0 = j;
            long index1 = j + step;
            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        }
    }
  #endif

  struct t_complex *temp = state->vector;
  state->vector = state->scratch_vector;
  state->scratch_vector = temp;
}

/**
 * Apply a 2-qubit quantum gate to the quantum state
 * @param state Quantum state vector
 * @param gate 4x4 gate matrix
 * @param control_qubit Control qubit index
 * @param target_qubit Target qubit index
 */
void q_apply_2q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int control_qubit, int target_qubit) {
  long size = state->size;
  long c_bit = 1L << control_qubit;
  long t_bit = 1L << target_qubit;
  long i;

  if (state == NULL || gate == NULL || control_qubit < 0 || target_qubit < 0 ||
      control_qubit >= state->qubits_num || target_qubit >= state->qubits_num ||
      control_qubit == target_qubit) {
    fprintf(stderr, "Error: Invalid arguments for 2-qubit gate application.\n");
    return;
  }

  #ifdef QCS_GPU_OPENCL
    q_apply_2q_gate_gpu(state, gate, control_qubit, target_qubit);
  #elif defined(QCS_CPU_OPENMP)
    c_copy_simd(state->scratch_vector, state->vector, size);
    
    #ifdef _OPENMP
    #pragma omp parallel for
    for (i = 0; i < size; i++) {
        if ((i & c_bit) != 0 && (i & t_bit) == 0) {
            long index0 = i;
            long index1 = i | t_bit;

            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        } else {
            state->scratch_vector[i] = state->vector[i];
        }
    }
    #else
    for (i = 0; i < size; i++) {
        if ((i & c_bit) != 0 && (i & t_bit) == 0) {
            long index0 = i;
            long index1 = i | t_bit;

            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        } else {
            state->scratch_vector[i] = state->vector[i];
        }
    }
    #endif
    
  #elif defined(QCS_GPU_OPENCL)
    c_copy_gpu_real(state->scratch_vector, state->vector, size);
    
    for (i = 0; i < size; i++) {
        if ((i & c_bit) != 0 && (i & t_bit) == 0) {
            long index0 = i;
            long index1 = i | t_bit;

            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        } else {
            state->scratch_vector[i] = state->vector[i];
        }
    }
    
  #elif defined(QCS_SIMD_ONLY)
    c_copy_simd(state->scratch_vector, state->vector, size);
    
    for (i = 0; i < size; i++) {
        if ((i & c_bit) != 0 && (i & t_bit) == 0) {
            long index0 = i;
            long index1 = i | t_bit;

            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        } else {
            state->scratch_vector[i] = state->vector[i];
        }
    }
    
  #elif defined(QCS_MULTI_THREAD)
    extern thread_pool_t *pool;
    
    memcpy(state->scratch_vector, state->vector, size * sizeof(struct t_complex));

    int effective_threads = (pool->num_threads > 4) ? 4 : pool->num_threads;
    long work_per_thread = (size + effective_threads - 1) / effective_threads;
    int k;

    for (k = 0; k < effective_threads; k++) {
        long start = k * work_per_thread;
        long end = (k + 1) * work_per_thread;
        if (end > size) end = size;
        
        if (start >= end) break;

        struct t_thread_args *args = malloc(sizeof(struct t_thread_args));
        if (!args) exit(EXIT_FAILURE);

        args->start = start;
        args->end = end;
        args->state = state;
        args->gate = gate;
        args->control_qubit = control_qubit;
        args->target_qubit = target_qubit;

        thread_pool_add_task(pool, q_apply_2q_gate_worker, args);
    }
    thread_pool_wait(pool);
    
  #else
    memcpy(state->scratch_vector, state->vector, size * sizeof(struct t_complex));

    for (i = 0; i < size; i++) {
        if ((i & c_bit) != 0 && (i & t_bit) == 0) {
            long index0 = i;
            long index1 = i | t_bit;

            struct t_complex v0 = state->vector[index0];
            struct t_complex v1 = state->vector[index1];

            state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
            state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
        } else {
            state->scratch_vector[i] = state->vector[i];
        }
    }
  #endif

  struct t_complex *temp = state->vector;
  state->vector = state->scratch_vector;
  state->scratch_vector = temp;
}

/**
 * Apply phase flip to a specific quantum state
 * @param state Quantum state vector
 * @param index Index of state to flip
 */
void q_apply_phase_flip(struct t_q_state *state, int index) {
  if (state == NULL || index < 0 || index >= state->size) {
    fprintf(stderr, "Error: Invalid state or index for phase flip.\n");
    return;
  }

  struct t_complex *temp = state->vector;
  state->vector = state->scratch_vector;
  state->scratch_vector = temp;

  state->vector[index].number_real = -state->scratch_vector[index].number_real;
  state->vector[index].number_imaginary = -state->scratch_vector[index].number_imaginary;
}

/**
 * Apply diffusion operator for Grover's algorithm
 * @param state Quantum state vector
 */
void q_apply_diffusion(struct t_q_state *state) {
  if (state == NULL) {
    fprintf(stderr, "Error: Invalid state for diffusion.\n");
    return;
  }

  long size = state->size;
  long i;
  struct t_complex mean = c_zero();
  double mean_magnitude;

  for (i = 0; i < size; i++) {
    mean = c_add(mean, state->vector[i]);
  }
  mean_magnitude = c_magnitude(mean);

  if (mean_magnitude > 1e-10) {
    mean.number_real /= mean_magnitude;
    mean.number_imaginary /= mean_magnitude;
  }

  struct t_complex two_mean = {2.0 * mean.number_real, 2.0 * mean.number_imaginary};
  
  struct t_complex *temp = state->vector;
  state->vector = state->scratch_vector;
  state->scratch_vector = temp;

  for (i = 0; i < size; i++) {
    state->vector[i].number_real = 
        two_mean.number_real - state->scratch_vector[i].number_real;
    state->vector[i].number_imaginary = 
        two_mean.number_imaginary - state->scratch_vector[i].number_imaginary;
  }
}

/**
 * Create identity gate matrix
 * @return Identity gate matrix
 */
struct t_q_matrix *q_gate_I(void) {
  struct t_q_matrix *i = q_matrix_init(2, 2);
  if (!i)
    return NULL;
  i->data[0] = c_one();
  i->data[3] = c_one();
  return i;
}

/**
 * Create X (Pauli-X) gate matrix
 * @return X gate matrix
 */
struct t_q_matrix *q_gate_X(void) {
  struct t_q_matrix *X = q_matrix_init(2, 2);
  if (!X)
    return NULL;
  X->data[1] = c_one();
  X->data[2] = c_one();
  return X;
}

/**
 * Create Hadamard gate matrix
 * @return Hadamard gate matrix
 */
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

/**
 * Create Y (Pauli-Y) gate matrix
 * @return Y gate matrix
 */
struct t_q_matrix *q_gate_Y(void) {
  struct t_q_matrix *Y = q_matrix_init(2, 2);
  if (!Y)
    return NULL;
  Y->data[1].number_imaginary = -1.0;
  Y->data[2].number_imaginary = 1.0;
  return Y;
}

/**
 * Create Z (Pauli-Z) gate matrix
 * @return Z gate matrix
 */
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

#ifdef QCS_MULTI_THREAD
static void q_apply_1q_gate_worker(void *arg) {
  struct t_thread_args *args = (struct t_thread_args *)arg;
  struct t_q_state *state = args->state;
  const struct t_q_matrix *gate = args->gate;
  int target_qubit = args->target_qubit;
  long start = args->start;
  long end = args->end;
  long step = 1L << target_qubit;
  long block_size = 1L << (target_qubit + 1);
  long i, j;

  for (i = start; i < end; i += block_size) {
    for (j = i; j < i + step && j < end; j++) {
      long index0 = j;
      long index1 = j + step;
      struct t_complex v0 = state->vector[index0];
      struct t_complex v1 = state->vector[index1];

      state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
      state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
    }
  }
  free(args);
}

static void q_apply_2q_gate_worker(void *arg) {
  struct t_thread_args *args = (struct t_thread_args *)arg;
  struct t_q_state *state = args->state;
  const struct t_q_matrix *gate = args->gate;
  int control_qubit = args->control_qubit;
  int target_qubit = args->target_qubit;
  long start = args->start;
  long end = args->end;
  long c_bit = 1L << control_qubit;
  long t_bit = 1L << target_qubit;
  long i;

  for (i = start; i < end; i++) {
    if ((i & c_bit) != 0 && (i & t_bit) == 0) {
      long index0 = i;
      long index1 = i | t_bit;

      struct t_complex v0 = state->vector[index0];
      struct t_complex v1 = state->vector[index1];

      state->scratch_vector[index0] = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
      state->scratch_vector[index1] = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
    } else {
      state->scratch_vector[i] = state->vector[i];
    }
  }
  free(args);
}
#endif