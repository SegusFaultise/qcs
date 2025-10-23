#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/qcs.h"
#include "internal.h"

#ifdef QCS_MULTI_THREAD
thread_pool_t *pool = NULL;
#endif

struct t_q_circuit {
  int num_qubits;
  int num_gates;
  struct t_q_state *state;
  char **gate_history;
  int *target_qubits;
  int *control_qubits;
  double *parameters;
  int history_size;
  int history_capacity;
};

/**
 * Create a new quantum circuit with specified number of qubits
 * @param num_qubits Number of qubits in the circuit
 * @return Pointer to created circuit or NULL on failure
 */
t_q_circuit *qc_create(int num_qubits) {
  #ifdef QCS_MULTI_THREAD
  if (pool == NULL) {
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    if (num_cores < 1)
      num_cores = 2;

    int effective_threads = (num_cores > 4) ? 4 : num_cores;
    pool = thread_pool_create(effective_threads, 16);

    if (pool == NULL) {
      fprintf(stderr, "Error: Could not create thread pool.\n");
      return NULL;
    }
  }
  #endif

  t_q_circuit *circuit = (t_q_circuit *)malloc(sizeof(t_q_circuit));
  if (!circuit)
    return NULL;

  circuit->num_qubits = num_qubits;
  circuit->num_gates = 0;
  circuit->state = q_state_init(num_qubits);
  circuit->history_size = 0;
  circuit->history_capacity = 100;

  circuit->gate_history =
      (char **)malloc(circuit->history_capacity * sizeof(char *));
  circuit->target_qubits =
      (int *)malloc(circuit->history_capacity * sizeof(int));
  circuit->control_qubits =
      (int *)malloc(circuit->history_capacity * sizeof(int));
  circuit->parameters =
      (double *)malloc(circuit->history_capacity * sizeof(double));

  return circuit;
}

/**
 * Destroy a quantum circuit and free all associated memory
 * @param circuit Circuit to destroy
 */
void qc_destroy(t_q_circuit *circuit) {
  #ifdef QCS_MULTI_THREAD
  if (pool != NULL) {
    thread_pool_destroy(pool);
    pool = NULL;
  }
  #endif

  int i;

  if (circuit) {
    if (circuit->state)
      q_state_free(circuit->state);
    if (circuit->gate_history) {
      for (i = 0; i < circuit->history_size; i++) {
        free(circuit->gate_history[i]);
      }
      free(circuit->gate_history);
    }
    if (circuit->target_qubits)
      free(circuit->target_qubits);
    if (circuit->control_qubits)
      free(circuit->control_qubits);
    if (circuit->parameters)
      free(circuit->parameters);
    free(circuit);
  }
}


/**
 * Duplicate a string with memory allocation
 * @param str String to duplicate
 * @return Duplicated string or NULL on failure
 */
static char *qc_strdup(const char *str) {
  size_t len;
  char *copy;
  
  if (str == NULL)
    return NULL;

  len = strlen(str) + 1;
  copy = (char *)malloc(len);
  if (copy) {
    memcpy(copy, str, len);
  }
  return copy;
}

/**
 * Add a gate to the circuit history
 * @param circuit Quantum circuit
 * @param gate_name Name of the gate
 * @param target Target qubit index
 * @param control Control qubit index (or -1 if none)
 * @param param Gate parameter value
 */
void qc_add_gate(t_q_circuit *circuit, const char *gate_name, int target,
                 int control, double param) {
  if (circuit->history_size >= circuit->history_capacity) {
    circuit->history_capacity *= 2;
    circuit->gate_history = (char **)realloc(
        circuit->gate_history, circuit->history_capacity * sizeof(char *));
    circuit->target_qubits = (int *)realloc(
        circuit->target_qubits, circuit->history_capacity * sizeof(int));
    circuit->control_qubits = (int *)realloc(
        circuit->control_qubits, circuit->history_capacity * sizeof(int));
    circuit->parameters = (double *)realloc(
        circuit->parameters, circuit->history_capacity * sizeof(double));
  }

  circuit->gate_history[circuit->history_size] = qc_strdup(gate_name);
  circuit->target_qubits[circuit->history_size] = target;
  circuit->control_qubits[circuit->history_size] = control;
  circuit->parameters[circuit->history_size] = param;
  circuit->history_size++;
  circuit->num_gates++;
}

/**
 * Apply Hadamard gate to specified qubit
 * @param circuit Quantum circuit
 * @param qubit Target qubit index
 */
void qc_h(t_q_circuit *circuit, int qubit) {
  struct t_q_matrix *H = q_gate_H();
  q_apply_1q_gate(circuit->state, H, qubit);
  q_matrix_free(H);
  qc_add_gate(circuit, "H", qubit, -1, 0.0);
}

/**
 * Apply X (Pauli-X) gate to specified qubit
 * @param circuit Quantum circuit
 * @param qubit Target qubit index
 */
void qc_x(t_q_circuit *circuit, int qubit) {
  struct t_q_matrix *X = q_gate_X();
  q_apply_1q_gate(circuit->state, X, qubit);
  q_matrix_free(X);
  qc_add_gate(circuit, "X", qubit, -1, 0.0);
}

/**
 * Apply CNOT (controlled-X) gate
 * @param circuit Quantum circuit
 * @param control Control qubit index
 * @param target Target qubit index
 */
void qc_cnot(t_q_circuit *circuit, int control, int target) {
  struct t_q_matrix *X = q_gate_X();

  q_apply_2q_gate(circuit->state, X, control, target);
  q_matrix_free(X);
  qc_add_gate(circuit, "CNOT", target, control, 0.0);
}

/**
 * Apply RX (rotation around X-axis) gate to specified qubit
 * @param circuit Quantum circuit
 * @param qubit Target qubit index
 * @param angle Rotation angle in radians
 */
void qc_rx(t_q_circuit *circuit, int qubit, double angle) {
  struct t_q_matrix *RX = q_gate_RX(angle);
  q_apply_1q_gate(circuit->state, RX, qubit);
  q_matrix_free(RX);
  qc_add_gate(circuit, "RX", qubit, -1, angle);
}

/**
 * Apply RY (rotation around Y-axis) gate to specified qubit
 * @param circuit Quantum circuit
 * @param qubit Target qubit index
 * @param angle Rotation angle in radians
 */
void qc_ry(t_q_circuit *circuit, int qubit, double angle) {
  struct t_q_matrix *RY = q_gate_RY(angle);
  q_apply_1q_gate(circuit->state, RY, qubit);
  q_matrix_free(RY);
  qc_add_gate(circuit, "RY", qubit, -1, angle);
}

/**
 * Apply RZ (rotation around Z-axis) gate to specified qubit
 * @param circuit Quantum circuit
 * @param qubit Target qubit index
 * @param angle Rotation angle in radians
 */
void qc_rz(t_q_circuit *circuit, int qubit, double angle) {
  struct t_q_matrix *RZ = q_gate_RZ(angle);
  q_apply_1q_gate(circuit->state, RZ, qubit);
  q_matrix_free(RZ);
  qc_add_gate(circuit, "RZ", qubit, -1, angle);
}

/**
 * Measure a single qubit and collapse the quantum state
 * @param circuit Quantum circuit
 * @param qubit Qubit to measure
 * @return Measured value (0 or 1)
 */
int qc_measure(t_q_circuit *circuit, int qubit) {
  long state_size;
  double prob_0;
  long i;
  double amp_real;
  double amp_imag;
  double random_val;

  if (circuit == NULL || circuit->state == NULL || qubit < 0 ||
      qubit >= circuit->num_qubits) {
    return 0;
  }

  state_size = circuit->state->size;
  prob_0 = 0.0;

  for (i = 0; i < state_size; i++) {
    if ((i & (1L << qubit)) == 0) {
      amp_real = circuit->state->vector[i].number_real;
      amp_imag = circuit->state->vector[i].number_imaginary;
      prob_0 += (amp_real * amp_real) + (amp_imag * amp_imag);
    }
  }

  random_val = rand() / (double)RAND_MAX;

  if (random_val <= prob_0) {
    for (i = 0; i < state_size; i++) {
      if ((i & (1L << qubit)) != 0) {
        circuit->state->vector[i].number_real = 0.0;
        circuit->state->vector[i].number_imaginary = 0.0;
      }
    }
    q_state_normalize(circuit->state);
    return 0;

  } else {
    for (i = 0; i < state_size; i++) {
      if ((i & (1L << qubit)) == 0) {
        circuit->state->vector[i].number_real = 0.0;
        circuit->state->vector[i].number_imaginary = 0.0;
      }
    }

    q_state_normalize(circuit->state);
    return 1;
  }
}

/**
 * Measure all qubits in the circuit
 * @param circuit Quantum circuit
 * @param results Array to store measurement results
 */
void qc_measure_all(t_q_circuit *circuit, int *results) {
  if (circuit == NULL || results == NULL)
    return;

  int i;

  for (i = 0; i < circuit->num_qubits; i++) {
    results[i] = qc_measure(circuit, i);
  }

  qc_add_gate(circuit, "MEASURE", -1, -1, 0.0);
}

/**
 * Print the quantum circuit gate history
 * @param circuit Quantum circuit to print
 */
void qc_print_circuit(t_q_circuit *circuit) {
  int q;
  int g;
  int has_measurement;

  printf("\n╔══════════════════════════════════════════╗\n");
  printf("║              QUANTUM CIRCUIT             ║\n");
  printf("╚══════════════════════════════════════════╝\n");
  printf("Qubits: %d | Gates: %d\n\n", circuit->num_qubits, circuit->num_gates);

  has_measurement = 0;

  for (g = 0; g < circuit->history_size; g++) {
    if (strcmp(circuit->gate_history[g], "MEASURE") == 0) {
      has_measurement = 1;
      break;
    }
  }

  for (q = 0; q < circuit->num_qubits; q++) {
    printf("q%-2d ", q);

    for (g = 0; g < circuit->history_size; g++) {
      const char *gate = circuit->gate_history[g];
      int target = circuit->target_qubits[g];
      int control = circuit->control_qubits[g];

      if (strcmp(gate, "H") == 0 && target == q) {
        printf("─H─");
      } else if (strcmp(gate, "X") == 0 && target == q) {
        printf("─X─");
      } else if (strcmp(gate, "CNOT") == 0) {
        if (control == q)
          printf("─∙─");
        else if (target == q)
          printf("─⊕─");
        else
          printf("───");
      } else {
        printf("───");
      }

      if (g < circuit->history_size - 1)
        printf("─");
    }
    if (!has_measurement) {
      printf("─M─");
    }
    printf("─┐\n");
  }

  printf("\nGATE SEQUENCE: ");
  for (g = 0; g < circuit->history_size; g++) {
    const char *gate = circuit->gate_history[g];
    int target = circuit->target_qubits[g];
    int control = circuit->control_qubits[g];

    if (strcmp(gate, "CNOT") == 0) {
      printf("CNOT(%d,%d) ", control, target);
    } else if (strcmp(gate, "MEASURE") == 0) {
      printf("MEASURE ");
    } else {
      printf("%s(%d) ", gate, target);
    }
  }
  printf("\n\n");
}

/**
 * Print the quantum state vector
 * @param circuit Quantum circuit
 * @param solution_index Index to highlight (or -1 for none)
 */
void qc_print_state(t_q_circuit *circuit, int solution_index) {
  q_state_print(circuit->state, solution_index);
}

/**
 * Get the probability amplitude for a specific quantum state
 * @param circuit Quantum circuit
 * @param state State index
 * @return Probability amplitude (0.0 to 1.0)
 */
double qc_get_probability(t_q_circuit *circuit, int state) {
  if (state < 0 || state >= circuit->state->size)
    return 0.0;
  return c_norm_sq(circuit->state->vector[state]);
}

/**
 * Apply Grover's search algorithm to find a specific quantum state
 * @param circuit Quantum circuit
 * @param solution_state Target state to search for
 */
void qc_grover_search(t_q_circuit *circuit, int solution_state) {
  int q;
  int i;
  int num_qubits = circuit->num_qubits;
  int iterations = q_grover_iterations(num_qubits);

  for (q = 0; q < circuit->num_qubits; q++) {
    qc_h(circuit, q);
  }

  for (i = 0; i < iterations; i++) {
    q_apply_phase_flip(circuit->state, solution_state);
    qc_add_gate(circuit, "ORACLE", solution_state, -1, 0.0);

    q_apply_diffusion(circuit->state);
    qc_add_gate(circuit, "DIFFUSION", -1, -1, 0.0);
  }
}

int qc_get_num_qubits(t_q_circuit *circuit) { return circuit->num_qubits; }
int qc_get_num_gates(t_q_circuit *circuit) { return circuit->num_gates; }

/**
 * Apply controlled phase gate
 * @param circuit Quantum circuit
 * @param control Control qubit index
 * @param target Target qubit index
 * @param angle Phase angle in radians
 */
void qc_cphase(t_q_circuit *circuit, int control, int target, double angle) {
  struct t_q_matrix *CP = q_gate_CP(angle);

  q_apply_2q_gate(circuit->state, CP, control, target);
  q_matrix_free(CP);
  qc_add_gate(circuit, "CPHASE", target, control, angle);
}

/**
 * Apply quantum Fourier transform to the circuit
 * @param circuit Quantum circuit
 */
void qc_quantum_fourier_transform(t_q_circuit *circuit) {
  double const m_pi = (3.14159265358979323846);

  int n = circuit->num_qubits;
  int i, j;

  for (i = 0; i < n; i++) {
    qc_h(circuit, i);

    for (j = i + 1; j < n; j++) {
      double angle = m_pi / (double)(1 << (j - i));
      qc_cphase(circuit, j, i, angle);
    }
  }
}

/**
 * Find the quantum state with the highest probability amplitude
 * @param circuit Quantum circuit
 * @return Index of the most likely state
 */
int qc_find_most_likely_state(t_q_circuit *circuit) {
  long max_idx = 0;
  double max_prob = 0.0;
  long num_states = circuit->state->size;
  long i;

  for (i = 0; i < num_states; i++) {
    double prob = qc_get_probability(circuit, i);
    if (prob > max_prob) {
      max_prob = prob;
      max_idx = i;
    }
  }
  return (int)max_idx;
}

/**
 * Apply Y (Pauli-Y) gate to specified qubit
 * @param circuit Quantum circuit
 * @param qubit Target qubit index
 */
void qc_y(t_q_circuit *circuit, int qubit) {
  struct t_q_matrix *Y = q_gate_Y();
  q_apply_1q_gate(circuit->state, Y, qubit);
  q_matrix_free(Y);
  qc_add_gate(circuit, "Y", qubit, -1, 0.0);
}

/**
 * Apply Z (Pauli-Z) gate to specified qubit
 * @param circuit Quantum circuit
 * @param qubit Target qubit index
 */
void qc_z(t_q_circuit *circuit, int qubit) {
  struct t_q_matrix *Z = q_gate_Z();
  q_apply_1q_gate(circuit->state, Z, qubit);
  q_matrix_free(Z);
  qc_add_gate(circuit, "Z", qubit, -1, 0.0);
}

/**
 * Apply phase gate to specified qubit
 * @param circuit Quantum circuit
 * @param qubit Target qubit index
 * @param angle Phase angle in radians
 */
void qc_phase(t_q_circuit *circuit, int qubit, double angle) {
  struct t_q_matrix *P = q_gate_P(angle);
  q_apply_1q_gate(circuit->state, P, qubit);
  q_matrix_free(P);
  qc_add_gate(circuit, "P", qubit, -1, angle);
}

/**
 * Create a GHZ (Greenberger-Horne-Zeilinger) entangled state
 * @param circuit Quantum circuit
 */
void qc_ghz_state(t_q_circuit *circuit) {
  int n = circuit->num_qubits;
  int i;

  if (n < 2) {
    fprintf(stderr, "Error: GHZ state requires at least 2 qubits.\n");
    return;
  }

  qc_h(circuit, 0);

  for (i = 0; i < n - 1; i++) {
    qc_cnot(circuit, i, i + 1);
  }
}

/**
 * Add a barrier to the quantum circuit
 * @param circuit Quantum circuit
 */
void qc_barrier(t_q_circuit *circuit) {
  qc_add_gate(circuit, "BARRIER", -1, -1, 0.0);
}

/**
 * Reset a qubit to the |0⟩ state
 * @param circuit Quantum circuit
 * @param qubit Qubit to reset
 */
void qc_reset(t_q_circuit *circuit, int qubit) {
  if (qc_measure(circuit, qubit) == 1) {
    qc_x(circuit, qubit);
  }
  qc_add_gate(circuit, "RESET", qubit, -1, 0.0);
}

/**
 * Run the quantum circuit and perform final measurements
 * @param circuit Quantum circuit
 */
void qc_run(t_q_circuit *circuit) {
  int *results = malloc(circuit->num_qubits * sizeof(int));
  if (results) {
    qc_measure_all(circuit, results);
    free(results);
  }
}

/**
 * Run multiple shots of the quantum circuit
 * @param circuit Quantum circuit
 * @param shots Number of shots to run
 * @param results Array to store shot results
 */
void qc_run_shots(t_q_circuit *circuit, int shots, int *results) {
  long i;
  int s;
  long num_states;
  double *probabilities;

  if (!circuit || shots <= 0 || !results)
    return;

  num_states = circuit->state->size;
  probabilities = malloc(num_states * sizeof(double));
  if (!probabilities)
    return;

  for (i = 0; i < num_states; i++) {
    probabilities[i] = qc_get_probability(circuit, i);
  }

  memset(results, 0, num_states * sizeof(int));

  for (s = 0; s < shots; s++) {
    double rand_val = rand() / (double)RAND_MAX;
    double cumulative_prob = 0.0;
    for (i = 0; i < num_states; i++) {
      cumulative_prob += probabilities[i];
      if (rand_val < cumulative_prob) {
        results[i]++;
        break;
      }
    }
  }
  free(probabilities);
}

/**
 * Implement Bernstein-Vazirani algorithm
 * @param circuit Quantum circuit
 * @param hidden_string Hidden string to find
 */
void qc_bernstein_vazirani(t_q_circuit *circuit, int hidden_string) {
  int n = circuit->num_qubits - 1;
  int i;

  if (n <= 0) {
    fprintf(stderr, "Bernstein-Vazirani requires at least 2 qubits (1 input + "
                    "1 ancilla).\n");
    return;
  }

  qc_x(circuit, n);
  qc_h(circuit, n);

  for (i = 0; i < n; i++) {
    qc_h(circuit, i);
  }

  qc_barrier(circuit);

  for (i = 0; i < n; i++) {
    if ((hidden_string >> i) & 1) {
      qc_cnot(circuit, i, n);
    }
  }

  qc_barrier(circuit);

  for (i = 0; i < n; i++) {
    qc_h(circuit, i);
  }
}

/**
 * Optimize the quantum circuit by removing redundant gates
 * @param circuit Quantum circuit to optimize
 */
void qc_optimize(t_q_circuit *circuit) {
  int i;
  char *gate1_name;
  int target1;
  int control1;
  char *gate2_name;
  int target2;
  int control2;
  int single_qubit_cancel;
  int cnot_cancel;
  int j;

  i = 0;
  while (i < circuit->history_size - 1) {
    gate1_name = circuit->gate_history[i];
    target1 = circuit->target_qubits[i];
    control1 = circuit->control_qubits[i];

    gate2_name = circuit->gate_history[i + 1];
    target2 = circuit->target_qubits[i + 1];
    control2 = circuit->control_qubits[i + 1];

    single_qubit_cancel =
        (target1 == target2) && (strcmp(gate1_name, gate2_name) == 0) &&
        (strcmp(gate1_name, "H") == 0 || strcmp(gate1_name, "X") == 0 ||
         strcmp(gate1_name, "Y") == 0 || strcmp(gate1_name, "Z") == 0);

    cnot_cancel = (strcmp(gate1_name, "CNOT") == 0) &&
                  (strcmp(gate2_name, "CNOT") == 0) &&
                  (target1 == target2) && (control1 == control2);

    if (single_qubit_cancel || cnot_cancel) {
      free(circuit->gate_history[i]);
      free(circuit->gate_history[i + 1]);
      for (j = i; j < circuit->history_size - 2; j++) {
        circuit->gate_history[j] = circuit->gate_history[j + 2];
        circuit->target_qubits[j] = circuit->target_qubits[j + 2];
        circuit->control_qubits[j] = circuit->control_qubits[j + 2];
        circuit->parameters[j] = circuit->parameters[j + 2];
      }
      circuit->history_size -= 2;
      circuit->num_gates -= 2;

      i = 0;
    } else {
      i++;
    }
  }
}
