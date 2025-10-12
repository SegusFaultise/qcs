#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_cnot() {
  printf("Testing: qc_cnot...\n");
  t_q_circuit *c = qc_create(2);
  qc_x(c, 0);       /* |10> */
  qc_cnot(c, 0, 1); /* |11> */
  assert(qc_find_most_likely_state(c) == 3);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
