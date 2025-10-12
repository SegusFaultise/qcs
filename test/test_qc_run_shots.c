#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_run_shots() {
  printf("Testing: qc_run_shots...\n");
  t_q_circuit *c = qc_create(1);
  int shots = 1000, results[2] = {0};
  qc_h(c, 0);
  qc_run_shots(c, shots, results);
  assert(results[0] + results[1] == shots);
  assert(results[0] > 400 && results[0] < 600);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
