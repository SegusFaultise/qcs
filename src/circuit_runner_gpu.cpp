#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "../include/circuit_runner.hpp"
#include "../include/gates_cuda.cuh"

void run_circuit(QuantumState &state, const CircuitOptions &options) {
  std::cout << "\n--- Running Quantum Circuit on GPU ---" << std::endl;

  static const std::map<std::string, int> operation_map = {
      {"H", 1}, {"X", 2}, {"Y", 3}, {"CNOT", 4}, {"GHZ", 100}};

  for (const auto &op : options.gates) {
    std::cout << "Executing (GPU): " << op.name << std::endl;
    int op_id = operation_map.count(op.name) ? operation_map.at(op.name) : 0;

    switch (op_id) {
    case 1:
      apply_H_gate_cuda(state, op.targets[0]);
      break;
    case 2:
      apply_X_gate_cuda(state, op.targets[0]);
      break;
    case 3: /* apply_Y_gate_cuda would go here */
      break;
    case 4: /* apply_CNOT_gate_cuda would go here */
      break;
    case 100: /* create_ghz_state_cuda would go here */
      break;
    default:
      std::cerr << "Warning: Unknown gate '" << op.name << "' found. Skipping."
                << std::endl;
      break;
    }
  }
  std::cout << "--- GPU Circuit Execution Finished ---\n" << std::endl;
}
