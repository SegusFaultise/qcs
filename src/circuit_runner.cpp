#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "../include/circuit_runner.hpp"
#include "../include/gates.hpp"

void run_circuit(QuantumState &state, const CircuitOptions &options) {
  std::cout << "\n--- Running Quantum Circuit ---" << std::endl;

  static const std::map<std::string, int> gate_map = {
      {"H", 1}, {"X", 2}, {"CNOT", 3}};

  for (const auto &op : options.gates) {
    std::cout << "Applying " << op.name << " gate to target(s): ";

    for (int t : op.targets) {
      std::cout << t << " ";
    }

    std::cout << std::endl;

    int gate_id = 0;
    auto it = gate_map.find(op.name);

    if (it != gate_map.end()) {
      gate_id = it->second;
    }

    switch (gate_id) {

    case 1: // H gate
      if (op.targets.size() == 1) {
        apply_H_gate(state, op.targets[0]);
      }
      break;

    case 2: // X gate
      if (op.targets.size() == 1) {
        apply_X_gate(state, op.targets[0]);
      }
      break;

    case 3: // CNOT gate
      if (op.targets.size() == 2) {
        apply_CNOT_gate(state, op.targets[0], op.targets[1]);
      }
      break;

    default:
      std::cerr << "Warning: Unknown gate '" << op.name << "' found. Skipping."
                << std::endl;
      break;
    }
  }
  std::cout << "--- Circuit Execution Finished ---\n" << std::endl;
}
