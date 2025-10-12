#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_x() {
  printf("Testing: qc_x...\n");
  t_q_circuit *c = qc_create(1);
  qc_x(c, 0);
  assert(qc_find_most_likely_state(c) == 1);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
