#ifndef QCS_H
#define QCS_H

/* Quantum Circuit Builder */
typedef struct t_q_circuit t_q_circuit;

/* Circuit Creation */
t_q_circuit *qc_create(int num_qubits);
void qc_destroy(t_q_circuit *circuit);

/* Basic Gates */
void qc_h(t_q_circuit *circuit, int qubit);
void qc_x(t_q_circuit *circuit, int qubit);
void qc_y(t_q_circuit *circuit, int qubit);
void qc_z(t_q_circuit *circuit, int qubit);
void qc_cnot(t_q_circuit *circuit, int control, int target);

/* Advanced Gates */
void qc_phase(t_q_circuit *circuit, int qubit, double angle);
void qc_rx(t_q_circuit *circuit, int qubit, double angle);
void qc_ry(t_q_circuit *circuit, int qubit, double angle);
void qc_rz(t_q_circuit *circuit, int qubit, double angle);

/* Circuit Operations */
void qc_barrier(t_q_circuit *circuit);
void qc_reset(t_q_circuit *circuit, int qubit);

/* Measurement */
int qc_measure(t_q_circuit *circuit, int qubit);
void qc_measure_all(t_q_circuit *circuit, int *results);

/* Circuit Execution */
void qc_run(t_q_circuit *circuit);
void qc_run_shots(t_q_circuit *circuit, int shots, int *results);

/* State Access */
double qc_get_probability(t_q_circuit *circuit, int state);
void qc_print_state(t_q_circuit *circuit);
void qc_print_circuit(t_q_circuit *circuit);

/* Built-in Algorithms */
void qc_grover_search(t_q_circuit *circuit, int solution_state, int iterations);
void qc_quantum_fourier_transform(t_q_circuit *circuit);
void qc_bernstein_vazirani(t_q_circuit *circuit, int hidden_string);

/* Utility Functions */
int qc_get_num_qubits(t_q_circuit *circuit);
int qc_get_num_gates(t_q_circuit *circuit);
void qc_optimize(t_q_circuit *circuit);

#endif
