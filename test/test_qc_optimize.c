#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_optimize() {
  printf("Testing: qc_optimize...\n");
  t_q_circuit *c = qc_create(2);
  qc_h(c, 0);
  qc_h(c, 0);
  int before = qc_get_num_gates(c);
  qc_optimize(c);
  int after = qc_get_num_gates(c);
  assert(after == before - 2);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
