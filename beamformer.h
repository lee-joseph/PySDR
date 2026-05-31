#ifndef BEAMFORMER_H
#define BEAMFORMER_H

#include <stdint.h>
#include <stdbool.h>

// ===== Configuration =====
#define NUM_ELEMENTS        4
#define SAMPLES_PER_FRAME   256
#define ADC_SAMPLE_RATE     1000000  // 1 MHz

// Fixed-point configuration
#define FP_BITS             14
#define FP_ONE              (1 << FP_BITS)

// Phase lookup table
#define PHASE_BITS          12
#define PHASE_MAX           (1 << PHASE_BITS)

// ===== Data Types =====

typedef struct {
    int32_t real;
    int32_t imag;
} complex_fp_t;

typedef struct {
    float target_theta;          // Target steering angle (radians)
    float d_over_wavelength;     // Antenna spacing / wavelength
    complex_fp_t weights[NUM_ELEMENTS];
    uint32_t frame_count;
} beamformer_state_t;

typedef struct {
    uint32_t power;              // Output power
    int16_t power_db;            // Output in dB
    uint16_t quality_percent;    // Beam quality (0-100)
} beamform_result_t;

// ===== Function Prototypes =====

// Initialization
void beamformer_init(float theta_rad, float d_over_lambda);
void phase_lut_init(void);

// Control
void beamformer_steer(float new_theta_rad);
void beamformer_set_spacing(float d_over_lambda);

// Processing
complex_fp_t beamform_sample(const int16_t *i_sample, const int16_t *q_sample);
beamform_result_t beamformer_process_frame(const int16_t *frame_buffer);

// Utilities
uint32_t complex_power(complex_fp_t c);
int16_t power_to_db(uint32_t power);

#endif // BEAMFORMER_H
