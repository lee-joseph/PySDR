/**
 * @file beamformer.h
 * @brief Core beamforming algorithms
 */

#ifndef BEAMFORMER_H
#define BEAMFORMER_H

#include <stdint.h>
#include "fixed_point_math.h"

typedef struct {
    float frequency_hz;
    float array_spacing_m;
    uint8_t num_elements;
    uint8_t beamforming_mode;  /* 0=Conventional, 1=MVDR */
    float target_theta_rad;
    float *phase_offsets;      /* Per-element calibration (radians) */
    float *amplitude_scale;    /* Per-element amplitude scaling */
} beamformer_config_t;

typedef struct {
    uint32_t power_db;
    uint32_t quality_percent;
    int32_t theta_estimate_rad;
    uint32_t frame_count;
} beamform_result_t;

typedef struct {
    uint8_t num_elements;
    uint8_t num_snapshots;
    uint8_t mode;
    float diagonal_load;
    complex_fp_t *weights;
    complex_fp_t *covariance;
    complex_fp_t *samples_buffer;
} beamformer_state_t;

/* Initialize beamformer */
int beamformer_init(const beamformer_config_t *cfg);

/* Process one frame and return results */
beamform_result_t beamformer_process_frame(
    const int16_t *samples,
    uint16_t num_samples
);

/* Steer beam to angle (radians) */
void beamformer_steer(float theta_rad);

/* Get current state */
beamformer_state_t* beamformer_get_state(void);

#endif // BEAMFORMER_H
