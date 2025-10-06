#include "internal.h"
#include <math.h>

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
