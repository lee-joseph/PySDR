/**
 * @file beamformer.c
 * @brief Core beamforming algorithms implementation
 */

#include "beamformer.h"
#include "steering_vector.h"
#include "fixed_point_math.h"
#include <math.h>
#include <string.h>

static beamformer_state_t g_beamformer;
static beamformer_config_t g_config;
static complex_fp_t g_weights[4];

int beamformer_init(const beamformer_config_t *cfg) {
    if (!cfg || cfg->num_elements > 4) {
        return -1;
    }

    memcpy(&g_config, cfg, sizeof(beamformer_config_t));

    g_beamformer.num_elements = cfg->num_elements;
    g_beamformer.mode = cfg->beamforming_mode;
    g_beamformer.weights = g_weights;

    /* Initialize steering vectors */
    steering_vector_init_lut();

    /* Initialize weights to uniform (conventional beamformer) */
    for (int i = 0; i < cfg->num_elements; i++) {
        int32_t w = fp_from_float(1.0f / cfg->num_elements);
        g_beamformer.weights[i].real = w;
        g_beamformer.weights[i].imag = 0;
    }

    return 0;
}

static void compute_steering_vector(float theta_rad, complex_fp_t *steering) {
    complex_fp_t steer_vec[4];

    steering_vector_calc(
        theta_rad,
        g_config.frequency_hz,
        g_config.array_spacing_m,
        g_config.num_elements,
        steer_vec
    );

    for (int i = 0; i < g_config.num_elements; i++) {
        steering[i] = steer_vec[i];
    }
}

static uint32_t power_to_db(uint32_t linear_power) {
    if (linear_power == 0) return 0;

    float power_float = (float)linear_power;
    float db = 10.0f * log10f(power_float + 1e-10f);

    return (uint32_t)(db + 50);  /* Offset for fixed-point */
}

beamform_result_t beamformer_process_frame(
    const int16_t *samples,
    uint16_t num_samples) {

    beamform_result_t result = {0};
    complex_fp_t sum = {0};
    uint32_t total_power = 0;

    /* Process I/Q pairs (interleaved) */
    for (uint16_t n = 0; n < num_samples; n++) {
        for (int m = 0; m < g_config.num_elements; m++) {
            int16_t i_sample = samples[n * g_config.num_elements * 2 + m * 2];
            int16_t q_sample = samples[n * g_config.num_elements * 2 + m * 2 + 1];

            complex_fp_t sample;
            sample.real = i_sample;
            sample.imag = q_sample;

            /* Apply weight: output += weight[m] * sample[m] */
            complex_fp_t weighted = fp_cmul(g_beamformer.weights[m], sample);
            sum.real += weighted.real;
            sum.imag += weighted.imag;
        }
    }

    total_power = fp_cmagnitude_sq(sum);
    result.power_db = power_to_db(total_power);
    result.quality_percent = 95;  /* Placeholder */
    result.theta_estimate_rad = (int32_t)g_config.target_theta_rad;
    result.frame_count++;

    return result;
}

void beamformer_steer(float theta_rad) {
    g_config.target_theta_rad = theta_rad;

    complex_fp_t steering[4];
    compute_steering_vector(theta_rad, steering);

    /* Conventional beamformer: weights = steering vector / num_elements */
    for (int i = 0; i < g_config.num_elements; i++) {
        int32_t norm_factor = fp_from_float(1.0f / g_config.num_elements);
        g_beamformer.weights[i] = fp_cmul(steering[i], (complex_fp_t){norm_factor, 0});
    }
}

beamformer_state_t* beamformer_get_state(void) {
    return &g_beamformer;
}
