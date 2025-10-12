#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_create_destroy() {
  printf("Testing: qc_create / qc_destroy...\n");
  t_q_circuit *c = qc_create(2);
  assert(c != NULL);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
