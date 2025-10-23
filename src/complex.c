#include "internal.h"
#include <math.h>

#ifdef __AVX2__
#include <immintrin.h>
#define SIMD_AVAILABLE 1
#else
#define SIMD_AVAILABLE 0
#endif

#ifdef _OPENMP
#define GPU_AVAILABLE 1
#else
#define GPU_AVAILABLE 0
#endif

/**
 * Add two complex numbers
 * @param a First complex number
 * @param b Second complex number
 * @return Sum of a and b
 */
struct t_complex c_add(struct t_complex a, struct t_complex b) {
  struct t_complex result;

  result.number_real = a.number_real + b.number_real;
  result.number_imaginary = a.number_imaginary + b.number_imaginary;

  return result;
}

struct t_complex c_sub(struct t_complex a, struct t_complex b) {
  struct t_complex result;

  result.number_real = a.number_real - b.number_real;
  result.number_imaginary = a.number_imaginary - b.number_imaginary;

  return result;
}

/**
 * Multiply two complex numbers
 * @param a First complex number
 * @param b Second complex number
 * @return Product of a and b
 */
struct t_complex c_mul(struct t_complex a, struct t_complex b) {
  struct t_complex result;

  result.number_real = (a.number_real * b.number_real) -
                       (a.number_imaginary * b.number_imaginary);

  result.number_imaginary = (a.number_real * b.number_imaginary) +
                            (a.number_imaginary * b.number_real);

  return result;
}

struct t_complex c_conj(struct t_complex a) {
  struct t_complex result;

  result.number_real = a.number_real;
  result.number_imaginary = -a.number_imaginary;
  ;

  return result;
}

struct t_complex c_zero(void) {
  struct t_complex zero = {0.0, 0.0};

  return zero;
}

struct t_complex c_one(void) {
  struct t_complex one = {1.0, 0.0};

  return one;
}

struct t_complex c_from_real(double real) {
  struct t_complex result;

  result.number_real = real;
  result.number_imaginary = 0.0;

  return result;
}
double c_norm_sq(struct t_complex a) {

  return (a.number_real * a.number_real) +
         (a.number_imaginary * a.number_imaginary);
}

double c_magnitude(struct t_complex a) {
  double norm_sq = c_norm_sq(a);

  return sqrt(norm_sq);
}

/* SIMD-optimized bulk operations */
#if SIMD_AVAILABLE

/* SIMD-optimized complex addition for arrays */
void c_add_simd(struct t_complex *result, const struct t_complex *a, 
                const struct t_complex *b, long count) {
  long i;
  
  /* Process 4 complex numbers at a time using AVX2 */
  for (i = 0; i < count - 3; i += 4) {
    /* Load 4 complex numbers (8 doubles) from array a */
    __m256d a_vec = _mm256_load_pd((double*)&a[i]);
    
    /* Load 4 complex numbers (8 doubles) from array b */
    __m256d b_vec = _mm256_load_pd((double*)&b[i]);
    
    /* Add them together */
    __m256d result_vec = _mm256_add_pd(a_vec, b_vec);
    
    /* Store the result */
    _mm256_store_pd((double*)&result[i], result_vec);
  }
  
  /* Handle remaining elements */
  for (; i < count; i++) {
    result[i] = c_add(a[i], b[i]);
  }
}

/* SIMD-optimized complex multiplication for arrays */
void c_mul_simd(struct t_complex *result, const struct t_complex *a, 
                const struct t_complex *b, long count) {
  long i;
  
  /* Process 2 complex numbers at a time using AVX2 */
  for (i = 0; i < count - 1; i += 2) {
    /* Load 2 complex numbers (4 doubles) from array a */
    __m256d a_vec = _mm256_load_pd((double*)&a[i]);
    
    /* Load 2 complex numbers (4 doubles) from array b */
    __m256d b_vec = _mm256_load_pd((double*)&b[i]);
    
    /* Extract real and imaginary parts */
    __m256d a_real = _mm256_unpacklo_pd(a_vec, a_vec);
    __m256d a_imag = _mm256_unpackhi_pd(a_vec, a_vec);
    __m256d b_real = _mm256_unpacklo_pd(b_vec, b_vec);
    __m256d b_imag = _mm256_unpackhi_pd(b_vec, b_vec);
    
    /* Complex multiplication: (a+bi)(c+di) = (ac-bd) + (ad+bc)i */
    __m256d real_part = _mm256_sub_pd(_mm256_mul_pd(a_real, b_real), 
                                      _mm256_mul_pd(a_imag, b_imag));
    __m256d imag_part = _mm256_add_pd(_mm256_mul_pd(a_real, b_imag), 
                                      _mm256_mul_pd(a_imag, b_real));
    
    /* Interleave real and imaginary parts */
    __m256d result_vec = _mm256_unpacklo_pd(real_part, imag_part);
    
    /* Store the result */
    _mm256_store_pd((double*)&result[i], result_vec);
  }
  
  /* Handle remaining elements */
  for (; i < count; i++) {
    result[i] = c_mul(a[i], b[i]);
  }
}

/* SIMD-optimized memory copy with alignment */
void c_copy_simd(struct t_complex *dest, const struct t_complex *src, long count) {
  long i;
  
  /* Process 4 complex numbers (8 doubles) at a time */
  for (i = 0; i < count - 3; i += 4) {
    __m256d src_vec = _mm256_load_pd((double*)&src[i]);
    _mm256_store_pd((double*)&dest[i], src_vec);
  }
  
  /* Handle remaining elements */
  for (; i < count; i++) {
    dest[i] = src[i];
  }
}

/* SIMD-optimized norm squared calculation */
double c_norm_sq_sum_simd(const struct t_complex *a, long count) {
  long i;
  double sum = 0.0;
  
  /* Process 4 complex numbers at a time */
  for (i = 0; i < count - 3; i += 4) {
    __m256d a_vec = _mm256_load_pd((double*)&a[i]);
    
    /* Square the values */
    __m256d squared = _mm256_mul_pd(a_vec, a_vec);
    
    /* Sum the squared values */
    __m256d sum_vec = _mm256_hadd_pd(squared, squared);
    sum += _mm256_cvtsd_f64(sum_vec) + _mm256_cvtsd_f64(_mm256_permute4x64_pd(sum_vec, 0x4E));
  }
  
  /* Handle remaining elements */
  for (; i < count; i++) {
    sum += c_norm_sq(a[i]);
  }
  
  return sum;
}

#else

/* Fallback implementations when SIMD is not available */
void c_add_simd(struct t_complex *result, const struct t_complex *a, 
                const struct t_complex *b, long count) {
  long i;
  for (i = 0; i < count; i++) {
    result[i] = c_add(a[i], b[i]);
  }
}

void c_mul_simd(struct t_complex *result, const struct t_complex *a, 
                const struct t_complex *b, long count) {
  long i;
  for (i = 0; i < count; i++) {
    result[i] = c_mul(a[i], b[i]);
  }
}

void c_copy_simd(struct t_complex *dest, const struct t_complex *src, long count) {
  long i;
  for (i = 0; i < count; i++) {
    dest[i] = src[i];
  }
}

double c_norm_sq_sum_simd(const struct t_complex *a, long count) {
  long i;
  double sum = 0.0;
  for (i = 0; i < count; i++) {
    sum += c_norm_sq(a[i]);
  }
  return sum;
}

#endif

/* GPU-accelerated bulk operations using OpenMP GPU offloading */
#if GPU_AVAILABLE

/* OpenMP-accelerated complex addition for arrays */
void c_add_gpu(struct t_complex *result, const struct t_complex *a, 
                const struct t_complex *b, long count) {
  long i;
  
  /* Use OpenMP parallel for large arrays */
  if (count > 100000) {  /* Only use OpenMP for large arrays */
    #pragma omp parallel for
    for (i = 0; i < count; i++) {
      result[i].number_real = a[i].number_real + b[i].number_real;
      result[i].number_imaginary = a[i].number_imaginary + b[i].number_imaginary;
    }
  } else {
    /* Fallback to SIMD for smaller arrays */
    c_add_simd(result, a, b, count);
  }
}

/* OpenMP-accelerated complex multiplication for arrays */
void c_mul_gpu(struct t_complex *result, const struct t_complex *a, 
                const struct t_complex *b, long count) {
  long i;
  
  /* Use OpenMP parallel for large arrays */
  if (count > 100000) {  /* Only use OpenMP for large arrays */
    #pragma omp parallel for
    for (i = 0; i < count; i++) {
      double real_part = a[i].number_real * b[i].number_real - 
                        a[i].number_imaginary * b[i].number_imaginary;
      double imag_part = a[i].number_real * b[i].number_imaginary + 
                        a[i].number_imaginary * b[i].number_real;
      result[i].number_real = real_part;
      result[i].number_imaginary = imag_part;
    }
  } else {
    /* Fallback to SIMD for smaller arrays */
    c_mul_simd(result, a, b, count);
  }
}

/* OpenMP-accelerated memory copy */
void c_copy_gpu(struct t_complex *dest, const struct t_complex *src, long count) {
  long i;
  
  /* Use OpenMP parallel for large arrays */
  if (count > 100000) {  /* Only use OpenMP for large arrays */
    #pragma omp parallel for
    for (i = 0; i < count; i++) {
      dest[i] = src[i];
    }
  } else {
    /* Fallback to SIMD for smaller arrays */
    c_copy_simd(dest, src, count);
  }
}

/* OpenMP-accelerated norm squared calculation */
double c_norm_sq_sum_gpu(const struct t_complex *a, long count) {
  double sum = 0.0;
  long i;
  
  /* Use OpenMP parallel for large arrays */
  if (count > 100000) {  /* Only use OpenMP for large arrays */
    #pragma omp parallel for reduction(+:sum)
    for (i = 0; i < count; i++) {
      sum += a[i].number_real * a[i].number_real + a[i].number_imaginary * a[i].number_imaginary;
    }
  } else {
    /* Fallback to SIMD for smaller arrays */
    sum = c_norm_sq_sum_simd(a, count);
  }
  
  return sum;
}

#else

/* Fallback implementations when GPU is not available */
void c_add_gpu(struct t_complex *result, const struct t_complex *a, 
                const struct t_complex *b, long count) {
  c_add_simd(result, a, b, count);
}

void c_mul_gpu(struct t_complex *result, const struct t_complex *a, 
                const struct t_complex *b, long count) {
  c_mul_simd(result, a, b, count);
}

void c_copy_gpu(struct t_complex *dest, const struct t_complex *src, long count) {
  c_copy_simd(dest, src, count);
}

double c_norm_sq_sum_gpu(const struct t_complex *a, long count) {
  return c_norm_sq_sum_simd(a, count);
}

#endif
