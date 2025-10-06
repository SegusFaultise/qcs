#define QCS_IMPLEMENTATION

#include "../qcs_single.h"

int main() {
  t_q_circuit *qc = qc_create(3);

  qc_h(qc, 0);
  qc_cnot(qc, 0, 1);
  qc_x(qc, 2);

  qc_print_circuit(qc);
  qc_print_state(qc);

  t_q_circuit *grover_circuit = qc_create(3);
  qc_grover_search(grover_circuit, 5, 10);
  qc_print_circuit(grover_circuit);
  qc_print_state(grover_circuit);

  qc_destroy(qc);
  qc_destroy(grover_circuit);

  return 0;
}
