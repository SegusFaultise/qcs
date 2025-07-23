#include <cstddef>
#include <iostream>

#include "../include/gates.hpp"
#include "../include/state.hpp"

void print_state(const QuantumState &state) {
  std::cout << "--- Quantum State ---\n";

  for (size_t i = 0; i < state.amplitudes.size(); ++i) {
    if (std::abs(state.amplitudes[i].real()) > 1e-9 ||
        std::abs(state.amplitudes[i].imag()) > 1e-9) {

      std::cout << "  |";

      for (int j = state.num_qubits - 1; j >= 0; --j) {
        std::cout << ((i >> j) & 1);
      }

      std::cout << "> : " << state.amplitudes[i] << std::endl;
    }
  }
  std::cout << "---------------------\n";
}

struct QuantumState init_state(int num_qbits) {
  struct QuantumState state;

  state.num_qubits = num_qbits;

  size_t num_amplitudes = 1ULL << num_qbits;

  state.amplitudes.resize(num_amplitudes);

  if (num_amplitudes > 0)
    state.amplitudes[0] = {1.0, 0.0};

  return state;
}
