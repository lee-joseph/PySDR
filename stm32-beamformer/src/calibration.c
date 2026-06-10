/**
 * @file calibration.c
 * @brief Calibration implementation
 */

#include "calibration.h"
#include "fixed_point_math.h"
#include <math.h>
#include <string.h>

int calibration_at_boresight(
    const int16_t *cal_samples,
    uint16_t num_samples,
    calibration_entry_t *cal_table) {

    if (!cal_samples || !cal_table || num_samples < 2) {
        return -1;
    }

    /* Process reference channel (element 0) */
    complex_fp_t ref_sum = {0};

    for (uint16_t n = 0; n < num_samples; n++) {
        int16_t i = cal_samples[n * 8];      /* Element 0, I */
        int16_t q = cal_samples[n * 8 + 1];  /* Element 0, Q */

        ref_sum.real += i;
        ref_sum.imag += q;
    }

    float ref_phase = atan2f((float)ref_sum.imag, (float)ref_sum.real);

    /* Compute phase and amplitude for each element relative to element 0 */
    for (int m = 0; m < 4; m++) {
        complex_fp_t elem_sum = {0};

        for (uint16_t n = 0; n < num_samples; n++) {
            int16_t i = cal_samples[n * 8 + m * 2];
            int16_t q = cal_samples[n * 8 + m * 2 + 1];

            elem_sum.real += i;
            elem_sum.imag += q;
        }

        float phase = atan2f((float)elem_sum.imag, (float)elem_sum.real);
        float mag = sqrtf((float)(elem_sum.real * elem_sum.real + elem_sum.imag * elem_sum.imag));

        cal_table[m].phase_rad = phase - ref_phase;
        cal_table[m].amplitude = mag / (sqrtf((float)(ref_sum.real * ref_sum.real + ref_sum.imag * ref_sum.imag)) + 1e-10f);
    }

    return 0;
}

void calibration_apply(
    int16_t *samples,
    uint16_t num_samples,
    const calibration_entry_t *cal_table,
    uint8_t num_elements) {

    if (!samples || !cal_table) return;

    for (uint16_t n = 0; n < num_samples; n++) {
        for (uint8_t m = 0; m < num_elements; m++) {
            int16_t i = samples[n * num_elements * 2 + m * 2];
            int16_t q = samples[n * num_elements * 2 + m * 2 + 1];

            /* Apply phase correction: rotate by -phase */
            float phase = cal_table[m].phase_rad;
            float cos_p = cosf(phase);
            float sin_p = sinf(phase);
            float amp = cal_table[m].amplitude;

            float i_corr = (i * cos_p + q * sin_p) / amp;
            float q_corr = (q * cos_p - i * sin_p) / amp;

            samples[n * num_elements * 2 + m * 2] = (int16_t)i_corr;
            samples[n * num_elements * 2 + m * 2 + 1] = (int16_t)q_corr;
        }
    }
}

float calibration_compute_phase_diff(
    const int16_t *ref_channel,
    const int16_t *test_channel,
    uint16_t num_samples) {

    if (!ref_channel || !test_channel || num_samples < 1) {
        return 0.0f;
    }

    int32_t ref_i_sum = 0, ref_q_sum = 0;
    int32_t test_i_sum = 0, test_q_sum = 0;

    for (uint16_t n = 0; n < num_samples; n++) {
        ref_i_sum += ref_channel[n * 2];
        ref_q_sum += ref_channel[n * 2 + 1];
        test_i_sum += test_channel[n * 2];
        test_q_sum += test_channel[n * 2 + 1];
    }

    float ref_phase = atan2f((float)ref_q_sum, (float)ref_i_sum);
    float test_phase = atan2f((float)test_q_sum, (float)test_i_sum);

    return test_phase - ref_phase;
}
