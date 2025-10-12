#include "../include/qcs.h"
#include <assert.h>
#include <stdio.h>

void test_qc_getters() {
  printf("Testing: qc_get_num_qubits / qc_get_num_gates...\n");
  t_q_circuit *c = qc_create(4);
  assert(qc_get_num_qubits(c) == 4);
  assert(qc_get_num_gates(c) == 0);
  qc_h(c, 0);
  assert(qc_get_num_gates(c) == 1);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
