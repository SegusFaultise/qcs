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
double c_magnitude(double real);

/* QUANTUM STATE */
struct t_q_state {
  int qubits_num;
  int size;

  struct t_complex *vector;
};

struct t_q_state *q_state_init(int qubits_num);
void q_state_free(struct t_q_state *state);
void q_state_set_basis(struct t_q_state *state, int index_basis);

/* QUANTUM MATRIX */
struct t_q_matrix {
  int rows;
  int cols;

  struct t_complex *data;
};

struct t_q_matrix *q_matrix_init(int rows, int cols);
void q_matrix_free(struct t_q_matrix *mat);
void q_gate_apply(struct t_q_state *state, const struct t_q_matrix *gate);

#endif
