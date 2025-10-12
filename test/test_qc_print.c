#include "../include/qcs.h"
#include <stdio.h>

void test_qc_print() {
  printf("Testing: qc_print_state / qc_print_circuit (smoke test)...\n");
  t_q_circuit *c = qc_create(1);
  qc_h(c, 0);
  qc_print_state(c, -1);
  qc_print_circuit(c);
  qc_destroy(c);
  printf("  [PASSED]\n");
}
