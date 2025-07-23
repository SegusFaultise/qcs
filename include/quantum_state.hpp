#ifndef QUANTUM_STATE_HPP
#define QUANTUM_STATE_HPP

#include <complex>
#include <vector>

struct QuantumState {
  int num_qubits;
  std::vector<std::complex<double>> amplitudes;
};

void print_state(const QuantumState &state);
struct QuantumState init_state(int num_qubits);

#endif // QUANTUM_STATE_HPP
