#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_grover_search() {
  printf("Testing: qc_grover_search...\n");
  t_q_circuit *c = qc_create(6);
  int solution = 42;
  qc_grover_search(c, solution);
  assert(qc_get_probability(c, solution) > 0.9);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
