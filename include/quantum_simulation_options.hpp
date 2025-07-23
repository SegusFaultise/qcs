#ifndef QUANTUM_SIMULATION_OPTIONS_HPP
#define QUANTUM_SIMULATION_OPTIONS_HPP

#include <optional>
#include <string>
#include <vector>

/**
 * @struct GateOp
 * @brief Represents a single gate operation in the quantum circuit.
 */
struct GateOp {
  std::string name;
  std::vector<int> targets;
};

/**
 * @struct CircuitOptions
 * @brief Holds the complete configuration for the quantum circuit simulation.
 */
struct CircuitOptions {
  int qubits = 0;
  std::vector<GateOp> gates;
};

/**
 * @brief Loads circuit options from a TOML configuration file.
 * @param file_path The path to the TOML file.
 * @return An std::optional<CircuitOptions> containing the struct if successful,
 * or std::nullopt if parsing fails.
 */
std::optional<CircuitOptions>
load_circuit_options(const std::string &file_path);

/**
 * @brief Prints the contents of a CircuitOptions struct.
 * @param options The struct to print.
 */
void print_options(const CircuitOptions &options);

#endif // QUANTUM_SIMULATION_OPTIONS_HPP
