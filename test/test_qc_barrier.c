#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_barrier() {
  printf("Testing: qc_barrier...\n");
  t_q_circuit *c = qc_create(1);
  qc_barrier(c);
  assert(qc_get_num_gates(c) == 1);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
