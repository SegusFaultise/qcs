#ifndef GATES_HPP
#define DEBUG

#include "./state.hpp"

constexpr double INV_SQRT2 = 1.0 / M_SQRT2;

// H|0> = 1/sqrt(2) * (|0> + |1>)
// H|1> = 1/sqrt(2) * (|0> - |1>)
constexpr std::complex<double> H_GATE[2][2] = {{INV_SQRT2, INV_SQRT2},
                                               {INV_SQRT2, -INV_SQRT2}};

// X|0> = |1>
// X|1> - |0>
constexpr std::complex<double> X_GATE[2][2] = {{0.0, 1.0}, {1.0, 0.0}};

// Y|0> = i|1>
// Y|1> = -i|0>
constexpr std::complex<double> Y_GATE[2][2] = {{{0.0, 0.0}, {0.0, -1.0}},
                                               {{0.0, 1.0}, {0.0, 0.0}}};

void apply_H_gate(struct QuantumState &state, int target_qubit);
void apply_X_gate(struct QuantumState &state, int target_qubit);
void apply_Y_gate(struct QuantumState &state, int target_qubit);
void apply_CNOT_gate(struct QuantumState &state, int control_qubit,
                     int target_qubit);

#endif // GATES_HPP
