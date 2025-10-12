#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void test_qc_rotations() {
  printf("Testing: qc_rx, qc_ry, qc_rz...\n");
  t_q_circuit *c = qc_create(1);
  /* RX(pi) = iX (up to phase) */
  qc_rx(c, 0, M_PI);
  assert(qc_find_most_likely_state(c) == 1);

  /* RY(pi) = iY (up to phase) */
  qc_ry(c, 0, M_PI); /* |1> -> -|0> */
  assert(qc_find_most_likely_state(c) == 0);

  /* RZ(pi) = iZ (up to phase) */
  qc_h(c, 0);
  qc_rz(c, 0, M_PI);
  qc_h(c, 0);
  assert(qc_find_most_likely_state(c) == 1);

  qc_destroy(c);
  printf("  [PASSED]\n");
}
