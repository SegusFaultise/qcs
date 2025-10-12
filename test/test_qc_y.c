#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_y() {
  printf("Testing: qc_y...\n");
  t_q_circuit *c = qc_create(1);
  qc_x(c, 0); /* Start in |1> */
  qc_y(c, 0); /* Y|1> = i|0>. Prob of |0> is 1. */
  assert(qc_find_most_likely_state(c) == 0);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
