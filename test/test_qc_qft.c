#include "../include/qcs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#define assert_float_equal(a, b) assert(fabs((a) - (b)) < 1e-6)

void test_qc_qft() {
  printf("Testing: qc_quantum_fourier_transform...\n");
  t_q_circuit *c = qc_create(3);
  qc_quantum_fourier_transform(c);
  long i, num_states = 1L << 3;
  for (i = 0; i < num_states; i++) {
    assert_float_equal(qc_get_probability(c, i), 1.0 / num_states);
  }
  qc_destroy(c);
  printf("  [PASSED]\n");
}
