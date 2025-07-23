#include <cstddef>

#include "../include/quantum_state.hpp"

struct QuantumState init_state(int num_qbits) {
  struct QuantumState state;

  state.num_qubits = num_qbits;

  size_t num_amplitudes = 1ULL << num_qbits;

  state.amplitudes.resize(num_amplitudes);

  if (num_amplitudes > 0)
    state.amplitudes[0] = {1.0, 0.0};

  return state;
}
