/**
 * @file fixed_point_math.c
 * @brief Fixed-point arithmetic implementation
 */

#include "fixed_point_math.h"

complex_fp_t fp_cmul(complex_fp_t a, complex_fp_t b) {
    complex_fp_t result;
    int64_t real = (int64_t)a.real * b.real - (int64_t)a.imag * b.imag;
    int64_t imag = (int64_t)a.real * b.imag + (int64_t)a.imag * b.real;

    result.real = (int32_t)(real >> 15);
    result.imag = (int32_t)(imag >> 15);

    return result;
}

complex_fp_t fp_cmul_conj(complex_fp_t a, complex_fp_t b) {
    complex_fp_t result;
    int64_t real = (int64_t)a.real * b.real + (int64_t)a.imag * b.imag;
    int64_t imag = (int64_t)a.imag * b.real - (int64_t)a.real * b.imag;

    result.real = (int32_t)(real >> 15);
    result.imag = (int32_t)(imag >> 15);

    return result;
}

uint32_t fp_cmagnitude_sq(complex_fp_t a) {
    int64_t mag_sq = (int64_t)a.real * a.real + (int64_t)a.imag * a.imag;
    return (uint32_t)(mag_sq >> 15);
}

uint32_t fp_cmagnitude(complex_fp_t a) {
    uint32_t mag_sq = fp_cmagnitude_sq(a);

    if (mag_sq == 0) return 0;

    uint32_t x = mag_sq;
    uint32_t result = x;

    for (int i = 0; i < 4; i++) {
        result = (result + x / result) >> 1;
    }

    return result;
}
