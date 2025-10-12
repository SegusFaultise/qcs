#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void test_qc_create_destroy();
void test_qc_getters();
void test_qc_h();
void test_qc_x();
void test_qc_y();
void test_qc_z();
void test_qc_cnot();
void test_qc_phase();
void test_qc_rotations();
void test_qc_barrier();
void test_qc_reset();
void test_qc_measure();
void test_qc_run();
void test_qc_run_shots();
void test_qc_state_access();
void test_qc_print();
void test_qc_grover_search();
void test_qc_qft();
void test_qc_bv();
void test_qc_optimize();

int main() {
  printf("======================================\n");
  printf("  RUNNING QUANTUM SIMULATOR TEST SUITE \n");
  printf("======================================\n\n");

  srand(time(NULL));

  test_qc_create_destroy();
  test_qc_getters();
  test_qc_h();
  test_qc_x();
  test_qc_y();
  test_qc_z();
  test_qc_cnot();
  test_qc_phase();
  test_qc_rotations();
  test_qc_barrier();
  test_qc_reset();
  test_qc_measure();
  test_qc_run();
  test_qc_run_shots();
  test_qc_state_access();
  test_qc_print();
  test_qc_grover_search();
  test_qc_qft();
  test_qc_bv();
  test_qc_optimize();

  printf("\n--------------------------------------\n");
  printf("  ALL TESTS PASSED SUCCESSFULLY! \n");
  printf("--------------------------------------\n");

  return 0;
}
