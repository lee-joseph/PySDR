/**
 * @file fixed_point_math.h
 * @brief Fixed-point arithmetic utilities (Q15 format)
 */

#ifndef FIXED_POINT_MATH_H
#define FIXED_POINT_MATH_H

#include <stdint.h>

typedef struct {
    int32_t real;
    int32_t imag;
} complex_fp_t;

/* Multiply two fixed-point values (Q15 × Q15 → Q15) */
static inline int32_t fp_mul(int32_t a, int32_t b) {
    int64_t prod = (int64_t)a * b;
    return (int32_t)(prod >> 15);
}

/* Complex multiplication: a × b */
complex_fp_t fp_cmul(complex_fp_t a, complex_fp_t b);

/* Complex multiplication by conjugate: a × conj(b) */
complex_fp_t fp_cmul_conj(complex_fp_t a, complex_fp_t b);

/* Complex magnitude squared: |a|² */
uint32_t fp_cmagnitude_sq(complex_fp_t a);

/* Complex magnitude: |a| */
uint32_t fp_cmagnitude(complex_fp_t a);

/* Convert float to Q15 */
static inline int32_t fp_from_float(float x) {
    return (int32_t)(x * 32767.0f);
}

/* Convert Q15 to float */
static inline float fp_to_float(int32_t x) {
    return (float)x / 32767.0f;
}

#endif // FIXED_POINT_MATH_H
