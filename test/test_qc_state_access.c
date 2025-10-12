#include "../include/qcs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define assert_float_equal(a, b) assert(fabs((a) - (b)) < 1e-6)

void test_qc_state_access() {
  printf("Testing: qc_get_probability / qc_find_most_likely_state...\n");
  t_q_circuit *c = qc_create(2);
  qc_h(c, 0);
  assert_float_equal(qc_get_probability(c, 0), 0.5);
  assert(qc_find_most_likely_state(c) == 0 ||
         qc_find_most_likely_state(c) == 2);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
