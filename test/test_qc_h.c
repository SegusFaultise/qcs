#include "../include/qcs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define assert_float_equal(a, b) assert(fabs((a) - (b)) < 1e-6)

void test_qc_h() {
  printf("Testing: qc_h...\n");
  t_q_circuit *c = qc_create(1);
  qc_h(c, 0);
  assert_float_equal(qc_get_probability(c, 0), 0.5);
  assert_float_equal(qc_get_probability(c, 1), 0.5);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
