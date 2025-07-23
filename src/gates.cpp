#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "../include/gates.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

void apply_H_gate(QuantumState &state, int target_qubit) {
  if (target_qubit < 0 || target_qubit >= state.num_qubits)
    throw std::out_of_range("H-gate target qubit index is out of range.");

  const size_t num_amplitudes = 1ULL << state.num_qubits;

#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (size_t i = 0; i < num_amplitudes; ++i) {
    if (!((i >> target_qubit) & 1)) {
      size_t j = i | (1ULL << target_qubit);
      auto c_i = state.amplitudes[i];
      auto c_j = state.amplitudes[j];
      state.amplitudes[i] = (c_i + c_j) * INV_SQRT2;
      state.amplitudes[j] = (c_i - c_j) * INV_SQRT2;
    }
  }
}

void apply_X_gate(QuantumState &state, int target_qubit) {
  if (target_qubit < 0 || target_qubit >= state.num_qubits)
    throw std::out_of_range("X-gate target qubit index is out of range.");

  const size_t num_amplitudes = 1ULL << state.num_qubits;

#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (size_t i = 0; i < num_amplitudes; ++i) {
    if (!((i >> target_qubit) & 1)) {
      size_t j = i | (1ULL << target_qubit);
      std::swap(state.amplitudes[i], state.amplitudes[j]);
    }
  }
}

void apply_Y_gate(QuantumState &state, int target_qubit) {
  if (target_qubit < 0 || target_qubit >= state.num_qubits)
    throw std::out_of_range("Y-gate target qubit index is out of range.");

  const std::complex<double> i(0.0, 1.0);
  const size_t num_amplitudes = 1ULL << state.num_qubits;

#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (size_t k = 0; k < num_amplitudes; ++k) {
    if (!((k >> target_qubit) & 1)) {
      size_t j = k | (1ULL << target_qubit);
      auto c_k = state.amplitudes[k];
      auto c_j = state.amplitudes[j];
      state.amplitudes[k] = -i * c_j;
      state.amplitudes[j] = i * c_k;
    }
  }
}

void apply_CNOT_gate(QuantumState &state, int control_qubit, int target_qubit) {
  if (control_qubit < 0 || control_qubit >= state.num_qubits ||
      target_qubit < 0 || target_qubit >= state.num_qubits ||
      control_qubit == target_qubit)

    throw std::invalid_argument(
        "Invalid control or target qubit for CNOT gate.");

  const size_t num_amplitudes = 1ULL << state.num_qubits;

#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (size_t i = 0; i < num_amplitudes; ++i) {
    if ((i >> control_qubit) & 1) {
      if (!((i >> target_qubit) & 1)) {
        size_t j = i | (1ULL << target_qubit);
        std::swap(state.amplitudes[i], state.amplitudes[j]);
      }
    }
  }
}

void create_GHZ_state(QuantumState &state) {
  if (state.num_qubits < 3)
    throw std::invalid_argument(
        "GHZ state creation requires at least 3 qubits.");

  std::cout << "\n--- Building GHZ State ---" << std::endl;

  apply_H_gate(state, 0);

  for (int i = 1; i < state.num_qubits; ++i) {
    apply_CNOT_gate(state, 0, i);
  }

  std::cout << "--- GHZ State Creation Complete ---\n" << std::endl;
}
