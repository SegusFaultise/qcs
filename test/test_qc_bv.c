#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void test_qc_bv() {
  printf("Testing: qc_bernstein_vazirani...\n");
  int n = 5;
  t_q_circuit *c = qc_create(n + 1);
  int secret = rand() % (1 << n);
  qc_bernstein_vazirani(c, secret);
  assert(qc_find_most_likely_state(c) == secret);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
