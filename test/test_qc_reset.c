#include "../include/qcs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

void test_qc_reset() {
  printf("Testing: qc_reset...\n");
  t_q_circuit *c = qc_create(1);
  qc_h(c, 0);
  qc_reset(c, 0);
  assert(qc_find_most_likely_state(c) == 0);
  assert(fabs(qc_get_probability(c, 0) - 1.0) < 1e-9);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
