#include <iostream>
#include <optional>

#include "../include/circuit_runner.hpp"
#include "../include/quantum_simulation_options.hpp"
#include "../include/quantum_state.hpp"

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

int main() {
  auto options_opt = load_circuit_options("quantum_circuit_config.toml");

  if (!options_opt) {
    std::cerr << "Failed to load configuration. Exiting." << std::endl;
    return 1;
  }

  CircuitOptions circuit_config = options_opt.value();
  print_options(circuit_config);

  QuantumState state = init_state(circuit_config.qubits);
  std::cout << "\nInitial State:" << std::endl;

  print_state(state);

  run_circuit(state, circuit_config);

  std::cout << "Final State:" << std::endl;
  print_state(state);

  return 0;
}
