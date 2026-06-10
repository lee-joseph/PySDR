/**
 * @file steering_vector.h
 * @brief Steering vector calculation declarations
 */

#ifndef STEERING_VECTOR_H
#define STEERING_VECTOR_H

#include <stdint.h>

typedef struct {
    int32_t real;
    int32_t imag;
} complex_fp_t;

void steering_vector_init_lut(void);
float get_wavelength(float freq_hz);
void steering_vector_calc(float theta_rad, float freq_hz, float spacing_m,
                         uint8_t num_elements, complex_fp_t *out_steering);

#endif // STEERING_VECTOR_H
