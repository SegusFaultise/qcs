#include "../include/qcs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

void test_qc_run() {
  printf("Testing: qc_run...\n");
  t_q_circuit *c = qc_create(1);
  qc_x(c, 0);
  qc_run(c);
  assert(qc_find_most_likely_state(c) == 1);
  assert(fabs(qc_get_probability(c, 1) - 1.0) < 1e-9);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
