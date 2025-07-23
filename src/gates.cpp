#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "../include/gates.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

/**
 * @brief Applies a Hadamard (H) gate to a target qubit.
 *
 * The H-gate transforms a qubit into an equal superposition of |0> and |1>,
 * forming the basis for many quantum algorithms.
 *
 * @param state The quantum state to modify (passed by reference).
 * @param target_qubit The index of the qubit to apply the gate to.
 */
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

/**
 * @brief Applies a Pauli-X (NOT) gate to a target qubit.
 *
 * The X-gate is the quantum equivalent of a classical NOT gate, flipping
 * the state of the target qubit (|0> becomes |1>, and |1> becomes |0>).
 *
 * @param state The quantum state to modify (passed by reference).
 * @param target_qubit The index of the qubit to flip.
 */
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

/**
 * @brief Applies a Pauli-Y gate to a target qubit.
 *
 * The Y-gate rotates the qubit state around the Y-axis of the Bloch sphere.
 * It maps |0> to i|1> and |1> to -i|0>.
 *
 * @param state The quantum state to modify (passed by reference).
 * @param target_qubit The index of the qubit to apply the gate to.
 */
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

/**
 * @brief Applies a Controlled-NOT (CNOT) gate.
 *
 * This gate performs an X-gate on the target qubit if and only if the
 * control qubit is in the state |1>. It is essential for creating entanglement.
 *
 * @param state The quantum state to modify (passed by reference).
 * @param control_qubit The index of the control qubit.
 * @param target_qubit The index of the target qubit.
 */
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
