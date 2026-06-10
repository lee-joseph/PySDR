/**
 * @file steering_vector.c
 * @brief Steering vector calculation for phased arrays
 */

#include <math.h>
#include <stdint.h>
#include "steering_vector.h"

#define M_PI 3.14159265358979323846
#define SPEED_OF_LIGHT 3e8

// Precomputed sine/cosine lookup table (4096 entries)
static int16_t sin_lut[4096];
static int16_t cos_lut[4096];
static int lut_initialized = 0;

void steering_vector_init_lut(void) {
    if (lut_initialized) return;

    for (int i = 0; i < 4096; i++) {
        float angle = 2.0f * M_PI * i / 4096.0f;
        sin_lut[i] = (int16_t)(32767.0f * sinf(angle));
        cos_lut[i] = (int16_t)(32767.0f * cosf(angle));
    }
    lut_initialized = 1;
}

float get_wavelength(float freq_hz) {
    return SPEED_OF_LIGHT / freq_hz;
}

void steering_vector_calc(float theta_rad, float freq_hz, float spacing_m,
                         uint8_t num_elements, complex_fp_t *out_steering) {
    if (!lut_initialized) steering_vector_init_lut();

    float wavelength = get_wavelength(freq_hz);
    float d_over_lambda = spacing_m / wavelength;

    for (int n = 0; n < num_elements; n++) {
        // Phase = 2π * d/λ * sin(θ) * n
        float phase_rad = 2.0f * M_PI * d_over_lambda * sinf(theta_rad) * n;

        // Normalize to [0, 2π)
        while (phase_rad >= 2.0f * M_PI) phase_rad -= 2.0f * M_PI;
        while (phase_rad < 0) phase_rad += 2.0f * M_PI;

        // Convert to LUT index
        uint16_t lut_idx = (uint16_t)((phase_rad / (2.0f * M_PI)) * 4096.0f);
        lut_idx = lut_idx & 0xFFF;  // Mask to 12 bits

        // Get sin/cos from LUT and convert to fixed-point
        out_steering[n].real = ((int32_t)cos_lut[lut_idx]) << 6;
        out_steering[n].imag = ((int32_t)sin_lut[lut_idx]) << 6;
    }
}
