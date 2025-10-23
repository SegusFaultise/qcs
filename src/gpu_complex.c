#include "internal.h"
#include <math.h>
#include <CL/cl.h>
#include <stdio.h>

static cl_context gpu_context = NULL;
static cl_command_queue gpu_queue = NULL;
static cl_device_id gpu_device = NULL;
static cl_program gpu_program = NULL;
static cl_kernel add_kernel = NULL;
static cl_kernel mul_kernel = NULL;
static cl_kernel copy_kernel = NULL;
static cl_kernel norm_kernel = NULL;
static cl_kernel gate_1q_kernel = NULL;
static cl_kernel gate_2q_kernel = NULL;

static cl_mem persistent_state_buffer = NULL;
static cl_mem persistent_scratch_buffer = NULL;
static long persistent_buffer_size = 0;
static int gpu_initialized = 0;
static const char* kernel_source = 
"__kernel void complex_add(__global float2* a, __global float2* b, __global float2* result, int count) {\n"
"    int i = get_global_id(0);\n"
"    if (i < count) {\n"
"        result[i].x = a[i].x + b[i].x;\n"
"        result[i].y = a[i].y + b[i].y;\n"
"    }\n"
"}\n"
"\n"
"__kernel void complex_mul(__global float2* a, __global float2* b, __global float2* result, int count) {\n"
"    int i = get_global_id(0);\n"
"    if (i < count) {\n"
"        float real = a[i].x * b[i].x - a[i].y * b[i].y;\n"
"        float imag = a[i].x * b[i].y + a[i].y * b[i].x;\n"
"        result[i].x = real;\n"
"        result[i].y = imag;\n"
"    }\n"
"}\n"
"\n"
"__kernel void complex_copy(__global float2* src, __global float2* dest, int count) {\n"
"    int i = get_global_id(0);\n"
"    if (i < count) {\n"
"        dest[i] = src[i];\n"
"    }\n"
"}\n"
"\n"
"__kernel void complex_norm_sum(__global float2* a, __global float* result, int count) {\n"
"    int i = get_global_id(0);\n"
"    if (i < count) {\n"
"        result[i] = a[i].x * a[i].x + a[i].y * a[i].y;\n"
"    }\n"
"}\n"
"\n"
"/* Quantum gate kernels */\n"
"__kernel void apply_1q_gate(__global float2* state, __global float2* scratch, \n"
"                           __global float2* gate_matrix, int target_qubit, int size) {\n"
"    int i = get_global_id(0);\n"
"    int step = 1 << target_qubit;\n"
"    int block_size = 1 << (target_qubit + 1);\n"
"    \n"
"    if (i < size) {\n"
"        int block_start = (i / block_size) * block_size;\n"
"        int j = block_start + (i % step);\n"
"        \n"
"        if (j < size) {\n"
"            int index0 = j;\n"
"            int index1 = j + step;\n"
"            \n"
"            if (index1 < size) {\n"
"                float2 v0 = state[index0];\n"
"                float2 v1 = state[index1];\n"
"                \n"
"                float2 gate0 = gate_matrix[0];\n"
"                float2 gate1 = gate_matrix[1];\n"
"                float2 gate2 = gate_matrix[2];\n"
"                float2 gate3 = gate_matrix[3];\n"
"                \n"
"                /* Apply gate matrix multiplication */\n"
"                float2 temp0, temp1;\n"
"                temp0.x = gate0.x * v0.x - gate0.y * v0.y + gate1.x * v1.x - gate1.y * v1.y;\n"
"                temp0.y = gate0.x * v0.y + gate0.y * v0.x + gate1.x * v1.y + gate1.y * v1.x;\n"
"                temp1.x = gate2.x * v0.x - gate2.y * v0.y + gate3.x * v1.x - gate3.y * v1.y;\n"
"                temp1.y = gate2.x * v0.y + gate2.y * v0.x + gate3.x * v1.y + gate3.y * v1.x;\n"
"                \n"
"                scratch[index0] = temp0;\n"
"                scratch[index1] = temp1;\n"
"            }\n"
"        }\n"
"    }\n"
"}\n"
"\n"
"__kernel void apply_2q_gate(__global float2* state, __global float2* scratch, \n"
"                           __global float2* gate_matrix, int control_qubit, int target_qubit, int size) {\n"
"    int i = get_global_id(0);\n"
"    int c_bit = 1 << control_qubit;\n"
"    int t_bit = 1 << target_qubit;\n"
"    \n"
"    if (i < size && (i & c_bit) != 0) {\n"
"        int index00 = i & ~t_bit;\n"
"        int index01 = i | t_bit;\n"
"        int index10 = (i & ~t_bit) | c_bit;\n"
"        int index11 = i;\n"
"        \n"
"        if (index00 < size && index01 < size && index10 < size && index11 < size) {\n"
"            float2 v00 = state[index00];\n"
"            float2 v01 = state[index01];\n"
"            float2 v10 = state[index10];\n"
"            float2 v11 = state[index11];\n"
"            \n"
"            /* Apply 4x4 gate matrix */\n"
"            float2 result00, result01, result10, result11;\n"
"            \n"
"            result00.x = gate_matrix[0].x * v00.x - gate_matrix[0].y * v00.y +\n"
"                        gate_matrix[1].x * v01.x - gate_matrix[1].y * v01.y +\n"
"                        gate_matrix[2].x * v10.x - gate_matrix[2].y * v10.y +\n"
"                        gate_matrix[3].x * v11.x - gate_matrix[3].y * v11.y;\n"
"            result00.y = gate_matrix[0].x * v00.y + gate_matrix[0].y * v00.x +\n"
"                        gate_matrix[1].x * v01.y + gate_matrix[1].y * v01.x +\n"
"                        gate_matrix[2].x * v10.y + gate_matrix[2].y * v10.x +\n"
"                        gate_matrix[3].x * v11.y + gate_matrix[3].y * v11.x;\n"
"            \n"
"            result01.x = gate_matrix[4].x * v00.x - gate_matrix[4].y * v00.y +\n"
"                        gate_matrix[5].x * v01.x - gate_matrix[5].y * v01.y +\n"
"                        gate_matrix[6].x * v10.x - gate_matrix[6].y * v10.y +\n"
"                        gate_matrix[7].x * v11.x - gate_matrix[7].y * v11.y;\n"
"            result01.y = gate_matrix[4].x * v00.y + gate_matrix[4].y * v00.x +\n"
"                        gate_matrix[5].x * v01.y + gate_matrix[5].y * v01.x +\n"
"                        gate_matrix[6].x * v10.y + gate_matrix[6].y * v10.x +\n"
"                        gate_matrix[7].x * v11.y + gate_matrix[7].y * v11.x;\n"
"            \n"
"            result10.x = gate_matrix[8].x * v00.x - gate_matrix[8].y * v00.y +\n"
"                        gate_matrix[9].x * v01.x - gate_matrix[9].y * v01.y +\n"
"                        gate_matrix[10].x * v10.x - gate_matrix[10].y * v10.y +\n"
"                        gate_matrix[11].x * v11.x - gate_matrix[11].y * v11.y;\n"
"            result10.y = gate_matrix[8].x * v00.y + gate_matrix[8].y * v00.x +\n"
"                        gate_matrix[9].x * v01.y + gate_matrix[9].y * v01.x +\n"
"                        gate_matrix[10].x * v10.y + gate_matrix[10].y * v10.x +\n"
"                        gate_matrix[11].x * v11.y + gate_matrix[11].y * v11.x;\n"
"            \n"
"            result11.x = gate_matrix[12].x * v00.x - gate_matrix[12].y * v00.y +\n"
"                        gate_matrix[13].x * v01.x - gate_matrix[13].y * v01.y +\n"
"                        gate_matrix[14].x * v10.x - gate_matrix[14].y * v10.y +\n"
"                        gate_matrix[15].x * v11.x - gate_matrix[15].y * v11.y;\n"
"            result11.y = gate_matrix[12].x * v00.y + gate_matrix[12].y * v00.x +\n"
"                        gate_matrix[13].x * v01.y + gate_matrix[13].y * v01.x +\n"
"                        gate_matrix[14].x * v10.y + gate_matrix[14].y * v10.x +\n"
"                        gate_matrix[15].x * v11.y + gate_matrix[15].y * v11.x;\n"
"            \n"
"            scratch[index00] = result00;\n"
"            scratch[index01] = result01;\n"
"            scratch[index10] = result10;\n"
"            scratch[index11] = result11;\n"
"        }\n"
"    }\n"
"}\n";

/**
 * Initialize OpenCL GPU context and create kernels for quantum operations
 * @return 1 on success, 0 on failure
 */
static int init_gpu_context(void) {
    cl_int err;
    cl_platform_id platforms[10];
    cl_uint num_platforms;
    int i;
    
    err = clGetPlatformIDs(10, platforms, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        return 0;
    }
    
    for (i = 0; i < num_platforms; i++) {
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &gpu_device, NULL);
        if (err == CL_SUCCESS) {
            break;
        }
    }
    
    if (err != CL_SUCCESS) {
        return 0;
    }
    
    gpu_context = clCreateContext(NULL, 1, &gpu_device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        return 0;
    }
    
    gpu_queue = clCreateCommandQueue(gpu_context, gpu_device, 0, &err);
    if (err != CL_SUCCESS) {
        return 0;
    }
    
    gpu_program = clCreateProgramWithSource(gpu_context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        return 0;
    }
    
    err = clBuildProgram(gpu_program, 1, &gpu_device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        return 0;
    }
    add_kernel = clCreateKernel(gpu_program, "complex_add", &err);
    if (err != CL_SUCCESS) return 0;
    
    mul_kernel = clCreateKernel(gpu_program, "complex_mul", &err);
    if (err != CL_SUCCESS) return 0;
    
    copy_kernel = clCreateKernel(gpu_program, "complex_copy", &err);
    if (err != CL_SUCCESS) return 0;
    
    norm_kernel = clCreateKernel(gpu_program, "complex_norm_sum", &err);
    if (err != CL_SUCCESS) return 0;
    
    gate_1q_kernel = clCreateKernel(gpu_program, "apply_1q_gate", &err);
    if (err != CL_SUCCESS) return 0;
    
    gate_2q_kernel = clCreateKernel(gpu_program, "apply_2q_gate", &err);
    if (err != CL_SUCCESS) return 0;
    
    return 1;
}

/**
 * Cleanup OpenCL resources and release all GPU memory
 */
static void cleanup_gpu_context(void) {
    if (persistent_scratch_buffer) clReleaseMemObject(persistent_scratch_buffer);
    if (persistent_state_buffer) clReleaseMemObject(persistent_state_buffer);
    if (gate_2q_kernel) clReleaseKernel(gate_2q_kernel);
    if (gate_1q_kernel) clReleaseKernel(gate_1q_kernel);
    if (norm_kernel) clReleaseKernel(norm_kernel);
    if (copy_kernel) clReleaseKernel(copy_kernel);
    if (mul_kernel) clReleaseKernel(mul_kernel);
    if (add_kernel) clReleaseKernel(add_kernel);
    if (gpu_program) clReleaseProgram(gpu_program);
    if (gpu_queue) clReleaseCommandQueue(gpu_queue);
    if (gpu_context) clReleaseContext(gpu_context);
    
    persistent_state_buffer = NULL;
    persistent_scratch_buffer = NULL;
    persistent_buffer_size = 0;
    gpu_initialized = 0;
}

/**
 * Initialize persistent GPU memory buffers for quantum state operations
 * @param size Size of the quantum state vector
 * @return 1 on success, 0 on failure
 */
static int init_persistent_gpu_memory(long size) {
    cl_int err;
    
    if (persistent_buffer_size >= size && persistent_state_buffer != NULL) {
        return 1;
    }
    
    if (persistent_state_buffer) {
        clReleaseMemObject(persistent_state_buffer);
        persistent_state_buffer = NULL;
    }
    if (persistent_scratch_buffer) {
        clReleaseMemObject(persistent_scratch_buffer);
        persistent_scratch_buffer = NULL;
    }
    persistent_state_buffer = clCreateBuffer(gpu_context, CL_MEM_READ_WRITE, 
                                           size * sizeof(struct t_complex), NULL, &err);
    if (err != CL_SUCCESS) return 0;
    
    persistent_scratch_buffer = clCreateBuffer(gpu_context, CL_MEM_READ_WRITE, 
                                             size * sizeof(struct t_complex), NULL, &err);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(persistent_state_buffer);
        persistent_state_buffer = NULL;
        return 0;
    }
    
    persistent_buffer_size = size;
    return 1;
}

/**
 * Upload quantum state vector to GPU memory
 * @param state Quantum state to upload
 * @return 1 on success, 0 on failure
 */
static int upload_state_to_gpu(struct t_q_state *state) {
    cl_int err;
    
    if (!init_persistent_gpu_memory(state->size)) {
        return 0;
    }
    
    err = clEnqueueWriteBuffer(gpu_queue, persistent_state_buffer, CL_TRUE, 0,
                              state->size * sizeof(struct t_complex), state->vector, 0, NULL, NULL);
    return (err == CL_SUCCESS);
}

/**
 * Download quantum state vector from GPU memory
 * @param state Quantum state to download to
 * @return 1 on success, 0 on failure
 */
static int download_state_from_gpu(struct t_q_state *state) {
    cl_int err;
    
    if (persistent_state_buffer == NULL) {
        return 0;
    }
    
    err = clEnqueueReadBuffer(gpu_queue, persistent_state_buffer, CL_TRUE, 0,
                             state->size * sizeof(struct t_complex), state->vector, 0, NULL, NULL);
    return (err == CL_SUCCESS);
}

/**
 * GPU-accelerated complex number addition using OpenCL
 * @param result Output array for results
 * @param a First input array
 * @param b Second input array
 * @param count Number of elements to process
 */
void c_add_gpu_real(struct t_complex *result, const struct t_complex *a, 
                    const struct t_complex *b, long count) {
    
    static int initialized = 0;
    if (!initialized) {
        if (!init_gpu_context()) {
            c_add_simd(result, a, b, count);
            return;
        }
        initialized = 1;
    }
    
    cl_int err;
    cl_mem a_buf, b_buf, result_buf;
    size_t global_size = count;
    size_t local_size = 64;
    
    /* Create buffers */
    a_buf = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          count * sizeof(float) * 2, (void*)a, &err);
    b_buf = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          count * sizeof(float) * 2, (void*)b, &err);
    result_buf = clCreateBuffer(gpu_context, CL_MEM_WRITE_ONLY, 
                               count * sizeof(float) * 2, NULL, &err);
    
    if (err != CL_SUCCESS) {
        c_add_simd(result, a, b, count);
        return;
    }
    
    clSetKernelArg(add_kernel, 0, sizeof(cl_mem), &a_buf);
    clSetKernelArg(add_kernel, 1, sizeof(cl_mem), &b_buf);
    clSetKernelArg(add_kernel, 2, sizeof(cl_mem), &result_buf);
    clSetKernelArg(add_kernel, 3, sizeof(int), &count);
    
    err = clEnqueueNDRangeKernel(gpu_queue, add_kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        c_add_simd(result, a, b, count);
        return;
    }
    
    clEnqueueReadBuffer(gpu_queue, result_buf, CL_TRUE, 0, count * sizeof(float) * 2, result, 0, NULL, NULL);
    
    clReleaseMemObject(a_buf);
    clReleaseMemObject(b_buf);
    clReleaseMemObject(result_buf);
}

void c_mul_gpu_real(struct t_complex *result, const struct t_complex *a, 
                    const struct t_complex *b, long count) {
    
    static int initialized = 0;
    if (!initialized) {
        if (!init_gpu_context()) {
            c_mul_simd(result, a, b, count);
            return;
        }
        initialized = 1;
    }
    
    cl_int err;
    cl_mem a_buf, b_buf, result_buf;
    size_t global_size = count;
    size_t local_size = 64;
    
    a_buf = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          count * sizeof(float) * 2, (void*)a, &err);
    b_buf = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          count * sizeof(float) * 2, (void*)b, &err);
    result_buf = clCreateBuffer(gpu_context, CL_MEM_WRITE_ONLY, 
                               count * sizeof(float) * 2, NULL, &err);
    
    if (err != CL_SUCCESS) {
        c_mul_simd(result, a, b, count);
        return;
    }
    
    clSetKernelArg(mul_kernel, 0, sizeof(cl_mem), &a_buf);
    clSetKernelArg(mul_kernel, 1, sizeof(cl_mem), &b_buf);
    clSetKernelArg(mul_kernel, 2, sizeof(cl_mem), &result_buf);
    clSetKernelArg(mul_kernel, 3, sizeof(int), &count);
    
    err = clEnqueueNDRangeKernel(gpu_queue, mul_kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        c_mul_simd(result, a, b, count);
        return;
    }
    
    clEnqueueReadBuffer(gpu_queue, result_buf, CL_TRUE, 0, count * sizeof(float) * 2, result, 0, NULL, NULL);
    
    clReleaseMemObject(a_buf);
    clReleaseMemObject(b_buf);
    clReleaseMemObject(result_buf);
}

void c_copy_gpu_real(struct t_complex *dest, const struct t_complex *src, long count) {
    
    static int initialized = 0;
    if (!initialized) {
        if (!init_gpu_context()) {
            c_copy_simd(dest, src, count);
            return;
        }
        initialized = 1;
    }
    
    cl_int err;
    cl_mem src_buf, dest_buf;
    size_t global_size = count;
    size_t local_size = 64;
    
    src_buf = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                            count * sizeof(float) * 2, (void*)src, &err);
    dest_buf = clCreateBuffer(gpu_context, CL_MEM_WRITE_ONLY, 
                             count * sizeof(float) * 2, NULL, &err);
    
    if (err != CL_SUCCESS) {
        c_copy_simd(dest, src, count);
        return;
    }
    
    clSetKernelArg(copy_kernel, 0, sizeof(cl_mem), &src_buf);
    clSetKernelArg(copy_kernel, 1, sizeof(cl_mem), &dest_buf);
    clSetKernelArg(copy_kernel, 2, sizeof(int), &count);
    
    err = clEnqueueNDRangeKernel(gpu_queue, copy_kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        c_copy_simd(dest, src, count);
        return;
    }
    
    clEnqueueReadBuffer(gpu_queue, dest_buf, CL_TRUE, 0, count * sizeof(float) * 2, dest, 0, NULL, NULL);
    
    clReleaseMemObject(src_buf);
    clReleaseMemObject(dest_buf);
}

double c_norm_sq_sum_gpu_real(const struct t_complex *a, long count) {
    
    static int initialized = 0;
    if (!initialized) {
        if (!init_gpu_context()) {
            return c_norm_sq_sum_simd(a, count);
        }
        initialized = 1;
    }
    
    cl_int err;
    cl_mem a_buf, result_buf;
    float *temp_result;
    double sum = 0.0;
    size_t global_size = count;
    size_t local_size = 64;
    long i;
    
    temp_result = malloc(count * sizeof(float));
    if (!temp_result) {
        return c_norm_sq_sum_simd(a, count);
    }
    
    a_buf = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          count * sizeof(float) * 2, (void*)a, &err);
    result_buf = clCreateBuffer(gpu_context, CL_MEM_WRITE_ONLY, 
                               count * sizeof(float), NULL, &err);
    
    if (err != CL_SUCCESS) {
        free(temp_result);
        return c_norm_sq_sum_simd(a, count);
    }
    
    clSetKernelArg(norm_kernel, 0, sizeof(cl_mem), &a_buf);
    clSetKernelArg(norm_kernel, 1, sizeof(cl_mem), &result_buf);
    clSetKernelArg(norm_kernel, 2, sizeof(int), &count);
    
    err = clEnqueueNDRangeKernel(gpu_queue, norm_kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        free(temp_result);
        return c_norm_sq_sum_simd(a, count);
    }
    
    clEnqueueReadBuffer(gpu_queue, result_buf, CL_TRUE, 0, count * sizeof(float), temp_result, 0, NULL, NULL);
    
    for (i = 0; i < count; i++) {
        sum += temp_result[i];
    }
    
    clReleaseMemObject(a_buf);
    clReleaseMemObject(result_buf);
    free(temp_result);
    
    return sum;
}

/**
 * GPU-accelerated 1-qubit quantum gate application using persistent memory
 * @param state Quantum state vector
 * @param gate 2x2 gate matrix
 * @param target_qubit Target qubit index
 */
void q_apply_1q_gate_gpu(struct t_q_state *state, const struct t_q_matrix *gate, int target_qubit) {
    long size = state->size;
    cl_int err;
    cl_mem gate_buf;
    size_t global_size = size;
    size_t local_size = 256; 
    
    if (state == NULL || gate == NULL || target_qubit < 0 || target_qubit >= state->qubits_num) {
        fprintf(stderr, "Error: Invalid arguments for 1-qubit gate application.\n");
        return;
    }
    
    if (!gpu_initialized) {
        if (!init_gpu_context()) {
            long step = 1L << target_qubit;
            long block_size = 1L << (target_qubit + 1);
            long i, j;
            
            for (i = 0; i < size; i += block_size) {
                for (j = i; j < i + step; j++) {
                    long index0 = j;
                    long index1 = j + step;
                    
                    struct t_complex v0 = state->vector[index0];
                    struct t_complex v1 = state->vector[index1];
                    
                    struct t_complex temp0, temp1;
                    temp0 = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
                    temp1 = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
                    
                    state->scratch_vector[index0] = temp0;
                    state->scratch_vector[index1] = temp1;
                }
            }
            
            for (i = 0; i < size; i++) {
                state->vector[i] = state->scratch_vector[i];
            }
            return;
        }
        gpu_initialized = 1;
    }
    
    if (!init_persistent_gpu_memory(size)) {
        long step = 1L << target_qubit;
        long block_size = 1L << (target_qubit + 1);
        long i, j;
        
        for (i = 0; i < size; i += block_size) {
            for (j = i; j < i + step; j++) {
                long index0 = j;
                long index1 = j + step;
                
                struct t_complex v0 = state->vector[index0];
                struct t_complex v1 = state->vector[index1];
                
                struct t_complex temp0, temp1;
                temp0 = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
                temp1 = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
                
                state->scratch_vector[index0] = temp0;
                state->scratch_vector[index1] = temp1;
            }
        }
        
        for (i = 0; i < size; i++) {
            state->vector[i] = state->scratch_vector[i];
        }
        return;
    }
    
    static long last_uploaded_size = 0;
    if (last_uploaded_size != size) {
        if (!upload_state_to_gpu(state)) {
            long step = 1L << target_qubit;
            long block_size = 1L << (target_qubit + 1);
            long i, j;
            
            for (i = 0; i < size; i += block_size) {
                for (j = i; j < i + step; j++) {
                    long index0 = j;
                    long index1 = j + step;
                    
                    struct t_complex v0 = state->vector[index0];
                    struct t_complex v1 = state->vector[index1];
                    
                    struct t_complex temp0, temp1;
                    temp0 = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
                    temp1 = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
                    
                    state->scratch_vector[index0] = temp0;
                    state->scratch_vector[index1] = temp1;
                }
            }
            
            for (i = 0; i < size; i++) {
                state->vector[i] = state->scratch_vector[i];
            }
            return;
        }
        last_uploaded_size = size;
    }
    
    gate_buf = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                             4 * sizeof(struct t_complex), gate->data, &err);
    if (err != CL_SUCCESS) {
        long step = 1L << target_qubit;
        long block_size = 1L << (target_qubit + 1);
        long i, j;
        
        for (i = 0; i < size; i += block_size) {
            for (j = i; j < i + step; j++) {
                long index0 = j;
                long index1 = j + step;
                
                struct t_complex v0 = state->vector[index0];
                struct t_complex v1 = state->vector[index1];
                
                struct t_complex temp0, temp1;
                temp0 = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
                temp1 = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
                
                state->scratch_vector[index0] = temp0;
                state->scratch_vector[index1] = temp1;
            }
        }
        
        for (i = 0; i < size; i++) {
            state->vector[i] = state->scratch_vector[i];
        }
        return;
    }
    
    clSetKernelArg(gate_1q_kernel, 0, sizeof(cl_mem), &persistent_state_buffer);
    clSetKernelArg(gate_1q_kernel, 1, sizeof(cl_mem), &persistent_scratch_buffer);
    clSetKernelArg(gate_1q_kernel, 2, sizeof(cl_mem), &gate_buf);
    clSetKernelArg(gate_1q_kernel, 3, sizeof(int), &target_qubit);
    clSetKernelArg(gate_1q_kernel, 4, sizeof(int), &size);
    
    err = clEnqueueNDRangeKernel(gpu_queue, gate_1q_kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(gate_buf);
        long step = 1L << target_qubit;
        long block_size = 1L << (target_qubit + 1);
        long i, j;
        
        for (i = 0; i < size; i += block_size) {
            for (j = i; j < i + step; j++) {
                long index0 = j;
                long index1 = j + step;
                
                struct t_complex v0 = state->vector[index0];
                struct t_complex v1 = state->vector[index1];
                
                struct t_complex temp0, temp1;
                temp0 = c_add(c_mul(gate->data[0], v0), c_mul(gate->data[1], v1));
                temp1 = c_add(c_mul(gate->data[2], v0), c_mul(gate->data[3], v1));
                
                state->scratch_vector[index0] = temp0;
                state->scratch_vector[index1] = temp1;
            }
        }
        
        for (i = 0; i < size; i++) {
            state->vector[i] = state->scratch_vector[i];
        }
        return;
    }
    
    cl_mem temp = persistent_state_buffer;
    persistent_state_buffer = persistent_scratch_buffer;
    persistent_scratch_buffer = temp;
    
    clReleaseMemObject(gate_buf);
}

/**
 * GPU-accelerated 2-qubit quantum gate application using persistent memory
 * @param state Quantum state vector
 * @param gate 4x4 gate matrix
 * @param control_qubit Control qubit index
 * @param target_qubit Target qubit index
 */
void q_apply_2q_gate_gpu(struct t_q_state *state, const struct t_q_matrix *gate, 
                         int control_qubit, int target_qubit) {
    long size = state->size;
    cl_int err;
    cl_mem gate_buf;
    size_t global_size = size;
    size_t local_size = 256;
    
    if (state == NULL || gate == NULL || control_qubit < 0 || target_qubit < 0 ||
        control_qubit >= state->qubits_num || target_qubit >= state->qubits_num) {
        fprintf(stderr, "Error: Invalid arguments for 2-qubit gate application.\n");
        return;
    }
    
    if (!gpu_initialized) {
        if (!init_gpu_context()) {
            long c_bit = 1L << control_qubit;
            long t_bit = 1L << target_qubit;
            long i;
            
            for (i = 0; i < size; i++) {
                if ((i & c_bit) != 0) {  
                    long index00 = i & ~t_bit;
                    long index01 = i | t_bit;
                    long index10 = (i & ~t_bit) | c_bit;
                    long index11 = i;
                    
                    if (index00 < size && index01 < size && index10 < size && index11 < size) {
                        struct t_complex v00 = state->vector[index00];
                        struct t_complex v01 = state->vector[index01];
                        struct t_complex v10 = state->vector[index10];
                        struct t_complex v11 = state->vector[index11];
                        
                        state->scratch_vector[index00] = c_add(c_add(c_mul(gate->data[0], v00), c_mul(gate->data[1], v01)), 
                                                              c_add(c_mul(gate->data[2], v10), c_mul(gate->data[3], v11)));
                        state->scratch_vector[index01] = c_add(c_add(c_mul(gate->data[4], v00), c_mul(gate->data[5], v01)), 
                                                              c_add(c_mul(gate->data[6], v10), c_mul(gate->data[7], v11)));
                        state->scratch_vector[index10] = c_add(c_add(c_mul(gate->data[8], v00), c_mul(gate->data[9], v01)), 
                                                              c_add(c_mul(gate->data[10], v10), c_mul(gate->data[11], v11)));
                        state->scratch_vector[index11] = c_add(c_add(c_mul(gate->data[12], v00), c_mul(gate->data[13], v01)), 
                                                              c_add(c_mul(gate->data[14], v10), c_mul(gate->data[15], v11)));
                    }
                }
            }
            
            for (i = 0; i < size; i++) {
                state->vector[i] = state->scratch_vector[i];
            }
            return;
        }
        gpu_initialized = 1;
    }
    
    if (!init_persistent_gpu_memory(size)) {
        long c_bit = 1L << control_qubit;
        long t_bit = 1L << target_qubit;
        long i;
        
        for (i = 0; i < size; i++) {
            if ((i & c_bit) != 0) { 
                long index00 = i & ~t_bit;
                long index01 = i | t_bit;
                long index10 = (i & ~t_bit) | c_bit;
                long index11 = i;
                
                if (index00 < size && index01 < size && index10 < size && index11 < size) {
                    struct t_complex v00 = state->vector[index00];
                    struct t_complex v01 = state->vector[index01];
                    struct t_complex v10 = state->vector[index10];
                    struct t_complex v11 = state->vector[index11];
                    
                    state->scratch_vector[index00] = c_add(c_add(c_mul(gate->data[0], v00), c_mul(gate->data[1], v01)), 
                                                          c_add(c_mul(gate->data[2], v10), c_mul(gate->data[3], v11)));
                    state->scratch_vector[index01] = c_add(c_add(c_mul(gate->data[4], v00), c_mul(gate->data[5], v01)), 
                                                          c_add(c_mul(gate->data[6], v10), c_mul(gate->data[7], v11)));
                    state->scratch_vector[index10] = c_add(c_add(c_mul(gate->data[8], v00), c_mul(gate->data[9], v01)), 
                                                          c_add(c_mul(gate->data[10], v10), c_mul(gate->data[11], v11)));
                    state->scratch_vector[index11] = c_add(c_add(c_mul(gate->data[12], v00), c_mul(gate->data[13], v01)), 
                                                          c_add(c_mul(gate->data[14], v10), c_mul(gate->data[15], v11)));
                }
            }
        }
        
        for (i = 0; i < size; i++) {
            state->vector[i] = state->scratch_vector[i];
        }
        return;
    }
    
    static long last_uploaded_size = 0;
    if (last_uploaded_size != size) {
        if (!upload_state_to_gpu(state)) {
            long c_bit = 1L << control_qubit;
            long t_bit = 1L << target_qubit;
            long i;
            
            for (i = 0; i < size; i++) {
                if ((i & c_bit) != 0) { 
                    long index00 = i & ~t_bit;
                    long index01 = i | t_bit;
                    long index10 = (i & ~t_bit) | c_bit;
                    long index11 = i;
                    
                    if (index00 < size && index01 < size && index10 < size && index11 < size) {
                        struct t_complex v00 = state->vector[index00];
                        struct t_complex v01 = state->vector[index01];
                        struct t_complex v10 = state->vector[index10];
                        struct t_complex v11 = state->vector[index11];
                        
                        state->scratch_vector[index00] = c_add(c_add(c_mul(gate->data[0], v00), c_mul(gate->data[1], v01)), 
                                                              c_add(c_mul(gate->data[2], v10), c_mul(gate->data[3], v11)));
                        state->scratch_vector[index01] = c_add(c_add(c_mul(gate->data[4], v00), c_mul(gate->data[5], v01)), 
                                                              c_add(c_mul(gate->data[6], v10), c_mul(gate->data[7], v11)));
                        state->scratch_vector[index10] = c_add(c_add(c_mul(gate->data[8], v00), c_mul(gate->data[9], v01)), 
                                                              c_add(c_mul(gate->data[10], v10), c_mul(gate->data[11], v11)));
                        state->scratch_vector[index11] = c_add(c_add(c_mul(gate->data[12], v00), c_mul(gate->data[13], v01)), 
                                                              c_add(c_mul(gate->data[14], v10), c_mul(gate->data[15], v11)));
                    }
                }
            }
            
            for (i = 0; i < size; i++) {
                state->vector[i] = state->scratch_vector[i];
            }
            return;
        }
        last_uploaded_size = size;
    }
    
    gate_buf = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                             16 * sizeof(struct t_complex), gate->data, &err);
    if (err != CL_SUCCESS) {
        long c_bit = 1L << control_qubit;
        long t_bit = 1L << target_qubit;
        long i;
        
        for (i = 0; i < size; i++) {
            if ((i & c_bit) != 0) { 
                long index00 = i & ~t_bit;
                long index01 = i | t_bit;
                long index10 = (i & ~t_bit) | c_bit;
                long index11 = i;
                
                if (index00 < size && index01 < size && index10 < size && index11 < size) {
                    struct t_complex v00 = state->vector[index00];
                    struct t_complex v01 = state->vector[index01];
                    struct t_complex v10 = state->vector[index10];
                    struct t_complex v11 = state->vector[index11];
                    
                    state->scratch_vector[index00] = c_add(c_add(c_mul(gate->data[0], v00), c_mul(gate->data[1], v01)), 
                                                          c_add(c_mul(gate->data[2], v10), c_mul(gate->data[3], v11)));
                    state->scratch_vector[index01] = c_add(c_add(c_mul(gate->data[4], v00), c_mul(gate->data[5], v01)), 
                                                          c_add(c_mul(gate->data[6], v10), c_mul(gate->data[7], v11)));
                    state->scratch_vector[index10] = c_add(c_add(c_mul(gate->data[8], v00), c_mul(gate->data[9], v01)), 
                                                          c_add(c_mul(gate->data[10], v10), c_mul(gate->data[11], v11)));
                    state->scratch_vector[index11] = c_add(c_add(c_mul(gate->data[12], v00), c_mul(gate->data[13], v01)), 
                                                          c_add(c_mul(gate->data[14], v10), c_mul(gate->data[15], v11)));
                }
            }
        }
        
        for (i = 0; i < size; i++) {
            state->vector[i] = state->scratch_vector[i];
        }
        return;
    }
    
    clSetKernelArg(gate_2q_kernel, 0, sizeof(cl_mem), &persistent_state_buffer);
    clSetKernelArg(gate_2q_kernel, 1, sizeof(cl_mem), &persistent_scratch_buffer);
    clSetKernelArg(gate_2q_kernel, 2, sizeof(cl_mem), &gate_buf);
    clSetKernelArg(gate_2q_kernel, 3, sizeof(int), &control_qubit);
    clSetKernelArg(gate_2q_kernel, 4, sizeof(int), &target_qubit);
    clSetKernelArg(gate_2q_kernel, 5, sizeof(int), &size);
    
    err = clEnqueueNDRangeKernel(gpu_queue, gate_2q_kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(gate_buf);
        long c_bit = 1L << control_qubit;
        long t_bit = 1L << target_qubit;
        long i;
        
        for (i = 0; i < size; i++) {
            if ((i & c_bit) != 0) { 
                long index00 = i & ~t_bit;
                long index01 = i | t_bit;
                long index10 = (i & ~t_bit) | c_bit;
                long index11 = i;
                
                if (index00 < size && index01 < size && index10 < size && index11 < size) {
                    struct t_complex v00 = state->vector[index00];
                    struct t_complex v01 = state->vector[index01];
                    struct t_complex v10 = state->vector[index10];
                    struct t_complex v11 = state->vector[index11];
                    
                    state->scratch_vector[index00] = c_add(c_add(c_mul(gate->data[0], v00), c_mul(gate->data[1], v01)), 
                                                          c_add(c_mul(gate->data[2], v10), c_mul(gate->data[3], v11)));
                    state->scratch_vector[index01] = c_add(c_add(c_mul(gate->data[4], v00), c_mul(gate->data[5], v01)), 
                                                          c_add(c_mul(gate->data[6], v10), c_mul(gate->data[7], v11)));
                    state->scratch_vector[index10] = c_add(c_add(c_mul(gate->data[8], v00), c_mul(gate->data[9], v01)), 
                                                          c_add(c_mul(gate->data[10], v10), c_mul(gate->data[11], v11)));
                    state->scratch_vector[index11] = c_add(c_add(c_mul(gate->data[12], v00), c_mul(gate->data[13], v01)), 
                                                          c_add(c_mul(gate->data[14], v10), c_mul(gate->data[15], v11)));
                }
            }
        }
        
        for (i = 0; i < size; i++) {
            state->vector[i] = state->scratch_vector[i];
        }
        return;
    }
    
    cl_mem temp = persistent_state_buffer;
    persistent_state_buffer = persistent_scratch_buffer;
    persistent_scratch_buffer = temp;
    
    clReleaseMemObject(gate_buf);
}
