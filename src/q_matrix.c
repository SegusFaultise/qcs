#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

struct t_q_matrix *q_matrix_init(int rows, int cols) {
  struct t_q_matrix *mat;
  int size = rows * cols;

  if (rows <= 0 || cols <= 0) {
    fprintf(stderr, "Error: Matrix dimensions must be postives\n");
    return NULL;
  }

  mat = (struct t_q_matrix *)malloc(sizeof(struct t_q_matrix));
  if (mat == NULL) {
    fprintf(stderr, "Error: Could not allocate t_q_matrix structure\n");
    return NULL;
  }

  mat->data = (struct t_complex *)calloc(size, sizeof(struct t_complex));
  if (mat->data == NULL) {
    fprintf(stderr, "Error: Could not allocate matrix data\n");
    return NULL;
  }

  mat->rows = rows;
  mat->cols = cols;

  return mat;
}

void q_matrix_free(struct t_q_matrix *mat) {
  if (mat) {
    if (mat->data) {
      free(mat->data);
    }
    free(mat);
  }
}

void q_gate_apply(struct t_q_state *state, const struct t_q_matrix *gate) {
  struct t_complex *new_vector;
  int i;
  int j;

  if (gate->cols != state->size || gate->rows != state->size) {
    fprintf(stderr, "Error: Gate dimenstions (%dx%d) mismatch size (%d)\n",
            gate->rows, gate->cols, state->size);
  }

  new_vector =
      (struct t_complex *)calloc(state->size, sizeof(struct t_complex));

  if (new_vector == NULL) {
    fprintf(stderr, "Error: Could not allocate temporay state vector for gate "
                    "application\n");
    return;
  }

  for (i = 0; i < state->size; i++) {
    struct t_complex sum = c_zero();

    for (j = 0; j < state->size; j++) {
      int k = i * gate->cols + j;
      struct t_complex term = c_mul(gate->data[k], state->vector[j]);

      sum = c_add(sum, term);
    }
  }

  free(state->vector);
  state->vector = new_vector;
}

void q_matrix_print(const struct t_q_matrix *mat) {
  int i;
  int j;

  printf("--- Quantum Gate Matrix (%d x %d) ---\n", mat->rows, mat->cols);
  for (i = 0; i < mat->rows; i++) {
    for (j = 0; j < mat->cols; j++) {
      int k = i * mat->cols + j;
      printf(" | %+.4f %+.4fi", mat->data[k].number_real,
             mat->data[k].number_imaginary);
    }
    printf(" |\n");
  }
  printf("---------------------------------------\n");
}
