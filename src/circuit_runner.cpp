#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "../include/circuit_runner.hpp"
#include "../include/gates.hpp"
#include "../include/state.hpp"

void run_circuit(QuantumState &state, const CircuitOptions &options) {
  std::cout << "\n--- Running Quantum Circuit ---" << std::endl;

  static const std::map<std::string, int> operation_map = {
      {"H", 1}, {"X", 2}, {"Y", 3}, {"CNOT", 4}, {"GHZ", 100}};

  for (const auto &op : options.gates) {
    std::cout << "Executing operation: " << op.name;

    if (!op.targets.empty()) {
      std::cout << " on target(s): ";
      for (int t : op.targets) {
        std::cout << t << " ";
      }
    }

    std::cout << std::endl;

    int operation_id = 0;
    auto it = operation_map.find(op.name);

    if (it != operation_map.end()) {
      operation_id = it->second;
    }

    switch (operation_id) {
    case 1: // H gate
      if (!op.targets.empty()) {
        apply_H_gate(state, op.targets[0]);
      }
      break;

    case 2: // X gate
      if (!op.targets.empty()) {
        apply_X_gate(state, op.targets[0]);
      }
      break;

    case 3: // Y gate
      if (!op.targets.empty()) {
        apply_Y_gate(state, op.targets[0]);
      }
      break;

    case 4: // CNOT gate
      if (op.targets.size() >= 2) {
        apply_CNOT_gate(state, op.targets[0], op.targets[1]);
      }
      break;

    case 100: // GHZ state creation
      create_GHZ_state(state);
      break;

    default:
      std::cerr << "Warning: Unknown operation '" << op.name
                << "' found. Skipping." << std::endl;
      break;
    }
  }
  std::cout << "--- Circuit Execution Finished ---\n" << std::endl;
}
