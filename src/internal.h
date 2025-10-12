#ifndef INTERNAL_H
#define INTERNAL_H

/* COMPLEX NUMBERS */
struct t_complex {
  double number_real;
  double number_imaginary;
};

struct t_complex c_add(struct t_complex a, struct t_complex b);
struct t_complex c_sub(struct t_complex a, struct t_complex b);
struct t_complex c_mul(struct t_complex a, struct t_complex b);
struct t_complex c_conj(struct t_complex a);
struct t_complex c_zero(void);
struct t_complex c_one(void);
struct t_complex c_from_real(double real);
double c_norm_sq(struct t_complex a);
double c_magnitude(struct t_complex a);

/* QUANTUM STATE VECTOR */
struct t_q_state {
  int qubits_num;
  long size;
  struct t_complex *vector;
  struct t_complex *scratch_vector;
};

struct t_q_state *q_state_init(int qubits_num);
void q_state_free(struct t_q_state *state);
void q_state_set_basis(struct t_q_state *state, int index_basis);
void q_state_print(const struct t_q_state *state, int solution_index);

/* QUANTUM MATRIX */
struct __attribute__((aligned(64))) t_q_matrix {
  int rows;
  int cols;
  struct t_complex *data;
};

struct t_q_matrix *q_matrix_init(int rows, int cols);
void q_matrix_free(struct t_q_matrix *mat);
void q_gate_apply(struct t_q_state *state, const struct t_q_matrix *gate);
void q_matrix_print(const struct t_q_matrix *mat);

/* QUANTUM GATES */
struct t_q_matrix *q_gate_I(void);
struct t_q_matrix *q_gate_X(void);
struct t_q_matrix *q_gate_H(void);
struct t_q_matrix *q_gate_CNOT(void);
struct t_q_matrix *q_gate_CP(double angle);
struct t_q_matrix *q_gate_P(double angle);
struct t_q_matrix *q_gate_Z(void);
struct t_q_matrix *q_gate_Y(void);

void q_apply_diffusion(struct t_q_state *state);
void q_apply_phase_flip(struct t_q_state *state, int target_index);
void q_apply_1q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int target_qubit);
void q_apply_2q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int control_qubit, int target_qubit);

/* QUANTUM UTILS */
void q_state_normalize(struct t_q_state *state);
int q_grover_iterations(int num_qubits);

/* PTHREADS THREAD POOL*/
#include <pthread.h>

struct t_task {
  void (*function)(void *arg);
  void *argument;
};

typedef struct {
  pthread_t *threads;
  struct t_task *task_queue;
  int num_threads;
  int queue_size;
  int head;
  int tail;
  int task_count;
  int active_tasks;
  int shutdown;

  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_cond_t all_tasks_done;
} thread_pool_t;

thread_pool_t *thread_pool_create(int num_threads, int queue_size);
int thread_pool_add_task(thread_pool_t *pool, void (*function)(void *),
                         void *arg);
void thread_pool_wait(thread_pool_t *pool);
int thread_pool_destroy(thread_pool_t *pool);
void get_thread_work_range(long total_size, int num_threads, int thread_id,
                           long *start, long *end);

/* PTHREADS THREAD ARGS*/
#define CACHE_LINE_SIZE 64

struct t_thread_args {
  long start;
  long end;

  int target_qubit;
  int control_qubit;

  struct t_complex mean;
  struct t_q_state *state;
  const struct t_q_matrix *gate;

  union {
    struct {
      double partial_real_sum;
      double partial_imag_sum;
    } sums;
    char pad[CACHE_LINE_SIZE];
  } reduction_result;

} __attribute__((aligned(CACHE_LINE_SIZE)));

#endif
