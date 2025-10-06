#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/qcs.h"
#include "internal.h"

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

t_q_circuit *qc_create(int num_qubits) {
  t_q_circuit *circuit = (t_q_circuit *)malloc(sizeof(t_q_circuit));
  if (!circuit)
    return NULL;

  circuit->num_qubits = num_qubits;
  circuit->num_gates = 0;
  circuit->state = q_state_init(num_qubits);
  circuit->history_size = 0;
  circuit->history_capacity = 100;

  q_state_set_basis(circuit->state, 0);

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

void qc_destroy(t_q_circuit *circuit) {
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

static char *qc_strdup(const char *str) {
  if (str == NULL)
    return NULL;

  size_t len = strlen(str) + 1;
  char *copy = (char *)malloc(len);
  if (copy) {
    memcpy(copy, str, len);
  }
  return copy;
}

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

void qc_h(t_q_circuit *circuit, int qubit) {
  struct t_q_matrix *H = q_gate_H();
  q_apply_1q_gate(circuit->state, H, qubit);
  q_matrix_free(H);
  qc_add_gate(circuit, "H", qubit, -1, 0.0);
}

void qc_x(t_q_circuit *circuit, int qubit) {
  struct t_q_matrix *X = q_gate_X();
  q_apply_1q_gate(circuit->state, X, qubit);
  q_matrix_free(X);
  qc_add_gate(circuit, "X", qubit, -1, 0.0);
}

void qc_cnot(t_q_circuit *circuit, int control, int target) {
  struct t_q_matrix *CNOT = q_gate_CNOT();
  q_apply_2q_gate(circuit->state, CNOT, control, target);
  q_matrix_free(CNOT);
  qc_add_gate(circuit, "CNOT", target, control, 0.0);
}

int qc_measure(t_q_circuit *circuit, int qubit) {
  if (circuit == NULL || circuit->state == NULL || qubit < 0 ||
      qubit >= circuit->num_qubits) {
    return 0;
  }

  long state_size = circuit->state->size;
  double prob_0 = 0.0;
  long i;

  for (i = 0; i < state_size; i++) {
    if ((i & (1L << qubit)) == 0) {
      double amp_real = circuit->state->vector[i].number_real;
      double amp_imag = circuit->state->vector[i].number_imaginary;
      prob_0 += (amp_real * amp_real) + (amp_imag * amp_imag);
    }
  }

  double random_val = rand() / (double)RAND_MAX;

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

void qc_measure_all(t_q_circuit *circuit, int *results) {
  if (circuit == NULL || results == NULL)
    return;

  int i;

  for (i = 0; i < circuit->num_qubits; i++) {
    results[i] = qc_measure(circuit, i);
  }

  qc_add_gate(circuit, "MEASURE", -1, -1, 0.0);
}

void qc_print_circuit(t_q_circuit *circuit) {
  int q;
  int g;

  printf("\n╔══════════════════════════════════════════╗\n");
  printf("║              QUANTUM CIRCUIT             ║\n");
  printf("╚══════════════════════════════════════════╝\n");
  printf("Qubits: %d | Gates: %d\n\n", circuit->num_qubits, circuit->num_gates);

  int has_measurement = 0;

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

void qc_print_state(t_q_circuit *circuit) { q_state_print(circuit->state, -1); }

double qc_get_probability(t_q_circuit *circuit, int state) {
  if (state < 0 || state >= circuit->state->size)
    return 0.0;
  return c_norm_sq(circuit->state->vector[state]);
}

void qc_grover_search(t_q_circuit *circuit, int solution_state,
                      int iterations) {
  int q;
  int i;

  printf("Running Grover's Search for |%d⟩ (%d iterations)\n", solution_state,
         iterations);

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

void qc_y(t_q_circuit *circuit, int qubit);
void qc_z(t_q_circuit *circuit, int qubit);
void qc_phase(t_q_circuit *circuit, int qubit, double angle);
void qc_barrier(t_q_circuit *circuit);
void qc_reset(t_q_circuit *circuit, int qubit);
void qc_run(t_q_circuit *circuit);
void qc_run_shots(t_q_circuit *circuit, int shots, int *results);
void qc_quantum_fourier_transform(t_q_circuit *circuit);
void qc_bernstein_vazirani(t_q_circuit *circuit, int hidden_string);
void qc_optimize(t_q_circuit *circuit);
