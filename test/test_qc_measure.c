#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_measure() {
  printf("Testing: qc_measure / qc_measure_all...\n");
  t_q_circuit *c = qc_create(2);
  int results[2];
  qc_x(c, 1); /* |01> */
  int r0 = qc_measure(c, 0);
  int r1 = qc_measure(c, 1);
  assert(r0 == 0 && r1 == 1);

  qc_measure_all(c, results);
  assert(results[0] == 0 && results[1] == 1);

  qc_destroy(c);
  printf("  [PASSED]\n");
}
