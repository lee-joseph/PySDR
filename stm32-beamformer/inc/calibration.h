/**
 * @file calibration.h
 * @brief Phase and amplitude calibration
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>
#include "fixed_point_math.h"

typedef struct {
    float phase_rad;
    float amplitude;
} calibration_entry_t;

/* Calibrate using known source at boresight */
int calibration_at_boresight(
    const int16_t *cal_samples,
    uint16_t num_samples,
    calibration_entry_t *cal_table
);

/* Apply calibration to samples */
void calibration_apply(
    int16_t *samples,
    uint16_t num_samples,
    const calibration_entry_t *cal_table,
    uint8_t num_elements
);

/* Compute phase difference between elements */
float calibration_compute_phase_diff(
    const int16_t *ref_channel,
    const int16_t *test_channel,
    uint16_t num_samples
);

#endif // CALIBRATION_H
