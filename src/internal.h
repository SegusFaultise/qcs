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
struct t_q_matrix *q_gate_oracle(int num_qubits, int solution_index);
struct t_q_matrix *q_gate_U0(int num_qubits);
struct t_q_matrix *q_gate_diffusion(int num_qubits);
void q_apply_diffusion(struct t_q_state *state);
void q_apply_phase_flip(struct t_q_state *state, int target_index);
void q_apply_1q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int target_qubit);
void q_apply_2q_gate(struct t_q_state *state, const struct t_q_matrix *gate,
                     int control_qubit, int target_qubit);

/* QUANTUM UTILS */
void q_state_normalize(struct t_q_state *state);
int q_grover_iterations(int num_qubits);

#endif
