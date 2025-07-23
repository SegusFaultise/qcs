#include <iostream>
#include <optional>

#include "../include/benchmark.hpp"
#include "../include/circuit_runner.hpp"
#include "../include/simulation_options.hpp"
#include "../include/state.hpp"

int main() {
  auto options_opt = load_circuit_options("quantum_circuit_config.toml");

  if (!options_opt) {
    std::cerr << "Failed to load configuration. Exiting." << std::endl;
    return 1;
  }

  CircuitOptions circuit_config = options_opt.value();
  print_options(circuit_config);

  QuantumState state = init_state(circuit_config.qubits);
  Timer timer;

  std::cout << "\nStarting circuit execution..." << std::endl;

  timer.start();
  run_circuit(state, circuit_config);

  double elapsed_ms = timer.stop();

  std::cout << "\nFinal State:" << std::endl;
  print_state(state);

  std::cout << "---------------------------" << std::endl;
  std::cout << "📊 Circuit execution took: " << elapsed_ms << " ms"
            << std::endl;
  std::cout << "---------------------------" << std::endl;

  return 0;
}
