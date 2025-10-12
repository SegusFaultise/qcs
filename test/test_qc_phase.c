#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void test_qc_phase() {
  printf("Testing: qc_phase...\n");
  t_q_circuit *c = qc_create(1);
  qc_x(c, 0);
  qc_phase(c, 0, M_PI); /* P(pi) = Z gate */
  assert(qc_find_most_likely_state(c) == 1);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
