#include "../include/gates_cuda.cuh"
#include <complex>
#include <cuComplex.h> // Use CUDA's complex number header
#include <cuda_runtime.h>
#include <iostream>
#include <stdexcept>
#include <vector>

// Helper macro to check for CUDA errors
#define CUDA_CHECK(err)                                                        \
  {                                                                            \
    if (err != cudaSuccess) {                                                  \
      throw std::runtime_error(std::string("CUDA Error: ") +                   \
                               cudaGetErrorString(err));                       \
    }                                                                          \
  }

// --- CUDA Kernels (Device Code) ---
// Note: All kernels now use cuDoubleComplex instead of std::complex<double>

__global__ void h_gate_kernel(cuDoubleComplex *amplitudes, int num_qubits,
                              int target_qubit) {
  size_t i = blockIdx.x * blockDim.x + threadIdx.x;
  size_t num_amplitudes = 1ULL << num_qubits;

  if (i < num_amplitudes) {
    if (!((i >> target_qubit) & 1)) {
      size_t j = i | (1ULL << target_qubit);

      cuDoubleComplex c_i = amplitudes[i];
      cuDoubleComplex c_j = amplitudes[j];

      const double inv_sqrt2 = 1.0 / 1.4142135623730951;

      // Use cuCadd and cuCsub for complex arithmetic if direct operators fail,
      // but modern CUDA often overloads them correctly.
      amplitudes[i] = make_cuDoubleComplex(cuCreal(c_i) + cuCreal(c_j),
                                           cuCimag(c_i) + cuCimag(c_j));
      amplitudes[i] = cuCmul(amplitudes[i], make_cuDoubleComplex(inv_sqrt2, 0));

      amplitudes[j] = make_cuDoubleComplex(cuCreal(c_i) - cuCreal(c_j),
                                           cuCimag(c_i) - cuCimag(c_j));
      amplitudes[j] = cuCmul(amplitudes[j], make_cuDoubleComplex(inv_sqrt2, 0));
    }
  }
}

__global__ void x_gate_kernel(cuDoubleComplex *amplitudes, int num_qubits,
                              int target_qubit) {
  size_t i = blockIdx.x * blockDim.x + threadIdx.x;
  size_t num_amplitudes = 1ULL << num_qubits;

  if (i < num_amplitudes) {
    if (!((i >> target_qubit) & 1)) {
      size_t j = i | (1ULL << target_qubit);
      // Replace std::swap with a manual swap
      cuDoubleComplex temp = amplitudes[i];
      amplitudes[i] = amplitudes[j];
      amplitudes[j] = temp;
    }
  }
}

__global__ void y_gate_kernel(cuDoubleComplex *amplitudes, int num_qubits,
                              int target_qubit) {
  size_t i = blockIdx.x * blockDim.x + threadIdx.x;
  size_t num_amplitudes = 1ULL << num_qubits;
  const cuDoubleComplex imag_i = make_cuDoubleComplex(0.0, 1.0);

  if (i < num_amplitudes) {
    if (!((i >> target_qubit) & 1)) {
      size_t j = i | (1ULL << target_qubit);
      cuDoubleComplex c_i = amplitudes[i];
      cuDoubleComplex c_j = amplitudes[j];
      amplitudes[i] = cuCmul(make_cuDoubleComplex(0.0, -1.0), c_j); // -i * c_j
      amplitudes[j] = cuCmul(imag_i, c_i);                          //  i * c_i
    }
  }
}

__global__ void cnot_gate_kernel(cuDoubleComplex *amplitudes, int num_qubits,
                                 int control_qubit, int target_qubit) {
  size_t i = blockIdx.x * blockDim.x + threadIdx.x;
  size_t num_amplitudes = 1ULL << num_qubits;

  if (i < num_amplitudes) {
    if (((i >> control_qubit) & 1) && !((i >> target_qubit) & 1)) {
      size_t j = i | (1ULL << target_qubit);
      cuDoubleComplex temp = amplitudes[i];
      amplitudes[i] = amplitudes[j];
      amplitudes[j] = temp;
    }
  }
}

// --- Host-Callable Wrappers (Host Code) ---
// This is a generic helper function to reduce code duplication
void launch_kernel(QuantumState &state, const void *kernel, int target_qubit,
                   int control_qubit = -1) {
  size_t num_amplitudes = state.amplitudes.size();
  size_t vector_size_bytes = num_amplitudes * sizeof(cuDoubleComplex);
  cuDoubleComplex *d_amplitudes;

  CUDA_CHECK(cudaMalloc(&d_amplitudes, vector_size_bytes));
  // Use reinterpret_cast because std::complex and cuDoubleComplex have
  // compatible memory layouts
  CUDA_CHECK(cudaMemcpy(d_amplitudes,
                        reinterpret_cast<const void *>(state.amplitudes.data()),
                        vector_size_bytes, cudaMemcpyHostToDevice));

  int threads_per_block = 256;
  int blocks_per_grid =
      (num_amplitudes + threads_per_block - 1) / threads_per_block;

  // Launch the correct kernel based on the function pointer
  if (kernel == (const void *)h_gate_kernel) {
    h_gate_kernel<<<blocks_per_grid, threads_per_block>>>(
        d_amplitudes, state.num_qubits, target_qubit);
  } else if (kernel == (const void *)x_gate_kernel) {
    x_gate_kernel<<<blocks_per_grid, threads_per_block>>>(
        d_amplitudes, state.num_qubits, target_qubit);
  } else if (kernel == (const void *)y_gate_kernel) {
    y_gate_kernel<<<blocks_per_grid, threads_per_block>>>(
        d_amplitudes, state.num_qubits, target_qubit);
  } else if (kernel == (const void *)cnot_gate_kernel) {
    cnot_gate_kernel<<<blocks_per_grid, threads_per_block>>>(
        d_amplitudes, state.num_qubits, control_qubit, target_qubit);
  }

  CUDA_CHECK(cudaDeviceSynchronize());
  CUDA_CHECK(cudaMemcpy(reinterpret_cast<void *>(state.amplitudes.data()),
                        d_amplitudes, vector_size_bytes,
                        cudaMemcpyDeviceToHost));
  CUDA_CHECK(cudaFree(d_amplitudes));
}

void apply_H_gate_cuda(QuantumState &state, int target_qubit) {
  launch_kernel(state, (const void *)h_gate_kernel, target_qubit);
}

void apply_X_gate_cuda(QuantumState &state, int target_qubit) {
  launch_kernel(state, (const void *)x_gate_kernel, target_qubit);
}

void apply_Y_gate_cuda(QuantumState &state, int target_qubit) {
  launch_kernel(state, (const void *)y_gate_kernel, target_qubit);
}

void apply_CNOT_gate_cuda(QuantumState &state, int control_qubit,
                          int target_qubit) {
  launch_kernel(state, (const void *)cnot_gate_kernel, target_qubit,
                control_qubit);
}

void create_ghz_state_cuda(QuantumState &state) {
  if (state.num_qubits < 3) {
    throw std::invalid_argument(
        "GHZ state creation requires at least 3 qubits.");
  }
  std::cout << "\n--- Building GHZ State on GPU ---" << std::endl;
  apply_H_gate_cuda(state, 0);
  for (int i = 1; i < state.num_qubits; ++i) {
    apply_CNOT_gate_cuda(state, 0, i);
  }
  std::cout << "--- GHZ State Creation Complete ---\n" << std::endl;
}
