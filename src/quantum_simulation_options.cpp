#include <iostream>

#include "../include/quantum_simulation_options.hpp"
#include "../lib/toml++/toml.hpp"

/**
 * @brief Implementation of the TOML loading function for the new format.
 */
std::optional<CircuitOptions>
load_circuit_options(const std::string &file_path) {
  toml::table config_data;

  try {
    config_data = toml::parse_file(file_path);
  } catch (const toml::parse_error &err) {
    std::cerr << "Error parsing file '" << file_path << "': " << err
              << std::endl;
    return std::nullopt;
  }

  CircuitOptions options;

  options.qubits = config_data["qubits"].value_or(0);

  if (options.qubits <= 0) {
    std::cerr << "Error: 'qubits' must be a positive number." << std::endl;
    return std::nullopt;
  }

  toml::array *gate_array = config_data["gates"].as_array();

  if (!gate_array) {
    std::cerr << "Error: [[gates]] array not found in config file."
              << std::endl;
    return std::nullopt;
  }

  for (toml::node &elem : *gate_array) {
    if (!elem.is_table())
      continue; // Skip if it's not a table

    toml::table &gate_table = *elem.as_table();
    GateOp op;

    op.name = gate_table["name"].value_or("");

    if (auto targets_node = gate_table["targets"].as_array()) {
      for (auto &target_elem : *targets_node) {
        op.targets.push_back(target_elem.value_or(-1));
      }
    }

    options.gates.push_back(op);
  }

  return options;
}

/**
 * @brief Implementation of the print function for the new struct format.
 */
void print_options(const CircuitOptions &options) {
  std::cout << "--- Circuit Configuration ---" << std::endl;
  std::cout << "Qubits: " << options.qubits << std::endl;
  std::cout << "Gates:" << std::endl;

  for (const auto &op : options.gates) {
    std::cout << "  - Name: " << op.name << ", Targets: [ ";

    for (int target : op.targets) {
      std::cout << target << " ";
    }

    std::cout << "]" << std::endl;
  }

  std::cout << "---------------------------" << std::endl;
}
