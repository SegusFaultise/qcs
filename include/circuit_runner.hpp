#ifndef QUANTUM_RUNNER_HPP
#define QUANTUM_RUNNER_HPP

#include "quantum_simulation_options.hpp"
#include "quantum_state.hpp"

void run_circuit(QuantumState &state, const CircuitOptions &options);

#endif // !QUANTUM_RUNNER_HPP
