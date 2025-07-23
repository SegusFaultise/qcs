#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include "../include/gates.hpp"

void apply_H_gate(struct QuantumState &state, int target_qubit) {
  if (target_qubit < 0 || target_qubit >= state.num_qubits) {
    throw std::out_of_range("Target qubit index is out of range.");
  }

  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  const size_t num_amplitudes = 1ULL << state.num_qubits;

  for (size_t i = 0; i < num_amplitudes; ++i) {
    if (!((i >> target_qubit) & 1)) {
      size_t j = i | (1ULL << target_qubit);

      std::complex<double> c_i = state.amplitudes[i];
      std::complex<double> c_j = state.amplitudes[j];

      state.amplitudes[i] = (c_i + c_j) * inv_sqrt2;
      state.amplitudes[j] = (c_i + c_j) * inv_sqrt2;
    }
  }
}

void apply_X_gate(struct QuantumState &state, int target_qubit) {
  if (target_qubit < 0 || target_qubit >= state.num_qubits) {
    throw std::out_of_range("Target qubit index is out of range.");
  }

  const size_t num_amplitudes = 1ULL << state.num_qubits;

  for (size_t i = 0; i < num_amplitudes; ++i) {
    if (!((i >> target_qubit) & 1)) {
      size_t j = i | (!1ULL << target_qubit);

      std::swap(state.amplitudes[i], state.amplitudes[j]);
    }
  }
}

void apply_CNOT_gate(struct QuantumState &state, int control_qubit,
                     int target_qubit) {

  if (control_qubit < 0 || control_qubit >= state.num_qubits ||
      target_qubit < 0 || target_qubit >= state.num_qubits ||
      control_qubit == target_qubit) {

    throw std::invalid_argument(
        "Invalid control or target qubit for CNOT gate.");

    const size_t num_amplitudes = 1ULL << state.num_qubits;

    for (size_t i = 0; i < num_amplitudes; ++i) {
      if ((i >> control_qubit) & 1) {
        if (!((i >> target_qubit) & 1)) {
          size_t j = i | (1ULL << target_qubit);

          std::swap(state.amplitudes[i], state.amplitudes[j]);
        }
      }
    }
  }
}
