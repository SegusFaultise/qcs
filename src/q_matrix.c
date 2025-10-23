#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

/**
 * Initialize a quantum matrix with specified dimensions
 * @param rows Number of rows
 * @param cols Number of columns
 * @return Pointer to allocated matrix or NULL on failure
 */
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

/**
 * Free memory allocated for a quantum matrix
 * @param mat Matrix to free
 */
void q_matrix_free(struct t_q_matrix *mat) {
  if (mat) {
    if (mat->data) {
      free(mat->data);
    }
    free(mat);
  }
}

#define BLOCK_SIZE 64

/**
 * Apply a quantum gate matrix to a quantum state vector
 * @param state Quantum state vector
 * @param gate Gate matrix to apply
 */
void q_gate_apply(struct t_q_state *state, const struct t_q_matrix *gate) {
  struct t_complex *new_vector = state->scratch_vector;
  struct t_complex *vector = state->vector;
  struct t_complex *temp;

  long N = state->size;
  long i, j;
  long ii, jj;
  long N_block = 0;

  if (gate->cols != N || gate->rows != N) {
    fprintf(stderr, "Error: Gate dimensions (%dx%d) mismatch size (%ld)\n",
            gate->rows, gate->cols, N);
    return;
  }

  for (i = 0; i < N; i++) {
    new_vector[i] = c_zero();
  }

  for (ii = 0; ii < N; ii += BLOCK_SIZE) {
    N_block = ((ii + BLOCK_SIZE) < N) ? BLOCK_SIZE : (N - ii);
    for (jj = 0; jj < N; jj += BLOCK_SIZE) {
      for (i = ii; i < ii + N_block; i++) {
        for (j = jj; j < jj + BLOCK_SIZE; j++) {
          if (j < N) {
            new_vector[i] =
                c_add(new_vector[i], c_mul(gate->data[i * N + j], vector[j]));
          }
        }
      }
    }
  }

  temp = state->vector;
  state->vector = new_vector;
  state->scratch_vector = temp;
}
