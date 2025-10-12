#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_z() {
  printf("Testing: qc_z...\n");
  t_q_circuit *c = qc_create(1);
  qc_h(c, 0); /* |+> */
  qc_z(c, 0); /* |-> */
  qc_h(c, 0); /* |1> */
  assert(qc_find_most_likely_state(c) == 1);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
