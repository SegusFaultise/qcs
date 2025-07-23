#ifndef GATES_CUDA_CUH
#define GATES_CUDA_CUH

#include "./state.hpp"

void apply_H_gate_cuda(QuantumState &state, int target_qubit);
void apply_X_gate_cuda(QuantumState &state, int target_qubit);
void apply_Y_gate_cuda(QuantumState &state, int target_qubit);
void apply_CNOT_gate_cuda(QuantumState &state, int control_qubit,
                          int target_qubit);

void create_ghz_state_cuda(QuantumState &state);

#endif // GATES_CUDA_CUH
