# Quantum Circuit Simulator
A high-performance quantum circuit simulator written in C++. This project provides both a single-threaded (classical) and a multi-threaded (parallel) version for simulating quantum circuits, with built-in benchmarking to compare their performance.

The simulator is configured via a simple TOML file, allowing users to define the number of qubits and the sequence of quantum gates to apply.

## Features
* **Dual Simulation Modes**:
    * **Classical**: A single-threaded implementation for correctness and baseline performance.
    * **Parallel**: A multi-threaded version using OpenMP for significant speedup on multi-core processors, especially for a high number of qubits.
* **Configurable Circuits**: Define quantum circuits easily using the `quantum_circuit_config.toml` file.
* **Core Quantum Gates Implemented**:
    * **H**: Hadamard Gate
    * **X**: Pauli-X (NOT) Gate
    * **Y**: Pauli-Y Gate
    * **CNOT**: Controlled-NOT Gate
* **High-Level Circuit Recipes**:
    * **GHZ**: A built-in function to generate a Greenberger–Horne–Zeilinger (GHZ) state.
* **Built-in Benchmarking**: Accurately measure and compare the execution time of the classical vs. parallel versions.

## Prerequisites
To build and run this project, you will need:
* A C++17 compliant compiler (e.g., `g++`)
* `make`
* OpenMP support in your compiler (for the parallel version)

## Building the Simulator

The `Makefile` provides several targets to build the different versions of the simulator.

* **Build both versions:**
    ```bash
    make all
    ```

* **Build only the classical (single-threaded) version:**
    ```bash
    make classical
    ```

* **Build only the parallel (multi-threaded) version:**
    ```bash
    make parallel
    ```

The compiled executables (`quantum_simulator_classical` and `quantum_simulator_parallel`) will be placed in the `build/` directory.

## Configuration

The simulation is controlled by the `quantum_circuit_config.toml` file.

* `qubits`: The total number of qubits in the simulation.
* `[[gates]]`: An array of tables, where each table represents a gate operation to be applied in sequence.
    * `name`: The name of the gate (e.g., "H", "X", "CNOT", "GHZ").
    * `targets`: An array of integers specifying the target qubit(s). For CNOT, the convention is `[control_qubit, target_qubit]`.

### Example `quantum_circuit_config.toml`

This configuration creates a 3-qubit GHZ state.

```toml
qubits = 24

[[gates]]
name = "H"
targets = [2]

[[gates]]
name = "X"
targets = [3]

[[gates]]
name = "Y"
targets = [1]

[[gates]]
name = "CNOT"
targets = [0, 1]

[[gates]]
name = "GHZ"
targets = [] 
