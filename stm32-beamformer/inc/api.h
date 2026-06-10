/**
 * @file beamformer.h
 * @brief Core beamforming API for STM32F446RE
 *
 * Provides conventional and MVDR adaptive beamforming for 4-element arrays.
 * All operations use fixed-point arithmetic for embedded efficiency.
 */

#ifndef BEAMFORMER_H
#define BEAMFORMER_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// ============================================================================
// Configuration Parameters (from inc/config.h)
// ============================================================================

#ifndef NUM_ELEMENTS
#define NUM_ELEMENTS 4
#endif

#ifndef SAMPLES_PER_FRAME
#define SAMPLES_PER_FRAME 256
#endif

#ifndef ADC_SAMPLE_RATE_HZ
#define ADC_SAMPLE_RATE_HZ 1000000  // 1 MHz
#endif

#ifndef CENTER_FREQUENCY_HZ
#define CENTER_FREQUENCY_HZ 2.8e9  // 2.8 GHz
#endif

#ifndef ARRAY_SPACING_M
#define ARRAY_SPACING_M 0.0535  // 0.5 wavelength at 2.8 GHz
#endif

#define SPEED_OF_LIGHT 3e8
#define WAVELENGTH (SPEED_OF_LIGHT / CENTER_FREQUENCY_HZ)

// Fixed-point configuration
#define FP_BITS 14
#define FP_ONE (1 << FP_BITS)

// Phase lookup table
#define PHASE_BITS 12
#define PHASE_MAX (1 << PHASE_BITS)

// ============================================================================
// Data Types
// ============================================================================

/** Complex number in fixed-point format */
typedef struct {
    int32_t real;
    int32_t imag;
} complex_fp_t;

/** Beamforming mode selection */
typedef enum {
    BEAMFORMER_CONVENTIONAL = 0,
    BEAMFORMER_MVDR = 1,
} beamformer_mode_t;

/** Beamformer configuration structure */
typedef struct {
    beamformer_mode_t mode;
    float frequency_hz;
    float array_spacing_m;
    uint8_t num_elements;
    float target_theta_rad;
    uint16_t num_snapshots;  // For MVDR covariance estimation
    float diagonal_load;     // Regularization for MVDR (0.01 typical)
} beamformer_config_t;

/** Beamformer state machine */
typedef struct {
    float target_theta;
    float d_over_wavelength;
    complex_fp_t weights[NUM_ELEMENTS];
    uint32_t frame_count;

    // MVDR state
    complex_fp_t cov_matrix[NUM_ELEMENTS * NUM_ELEMENTS];
    uint8_t cov_ready;
} beamformer_state_t;

/** Beamforming output result */
typedef struct {
    uint32_t power;         // Raw power output
    int16_t power_db;       // Power in dB
    uint16_t quality_percent; // 0-100, beam quality metric
    float snr_db;           // Estimated SNR
} beamform_result_t;

/** Calibration data: phase and amplitude correction per element */
typedef struct {
    int16_t phase;     // Phase correction in LUT indices
    int16_t amplitude; // Amplitude correction (fixed-point)
} complex_cal_t;

// ============================================================================
// Core Beamformer API
// ============================================================================

/**
 * @brief Initialize beamformer with configuration
 * @param cfg Configuration structure
 * @return 0 on success, -1 on error
 */
int beamformer_init(const beamformer_config_t *cfg);

/**
 * @brief Process one frame of ADC samples
 * @param samples Buffer of ADC samples (I0,Q0,I1,Q1,...,I3,Q3 format)
 * @param num_samples Number of I/Q pairs in frame
 * @return Beamforming result with power and metrics
 */
beamform_result_t beamformer_process_frame(
    const int16_t *samples,
    uint16_t num_samples
);

/**
 * @brief Process frame with MVDR beamforming
 * @param samples ADC buffer
 * @param num_samples Number of samples
 * @param covariance_matrix Covariance matrix (pre-computed)
 * @return Beamforming result
 */
beamform_result_t beamformer_process_frame_mvdr(
    const int16_t *samples,
    uint16_t num_samples,
    const complex_fp_t *covariance_matrix
);

/**
 * @brief Steer beam to new direction
 * @param theta_rad Target azimuth angle in radians (-π to π)
 */
void beamformer_steer(float theta_rad);

/**
 * @brief Get current beam steering angle
 * @return Current theta in radians
 */
float beamformer_get_steering_angle(void);

// ============================================================================
// Calibration API
// ============================================================================

/**
 * @brief Calibrate array using known source at boresight
 * @param cal_samples Buffer of calibration samples
 * @param num_samples Number of samples to use
 * @param cal_table Output: [NUM_ELEMENTS] calibration values
 * @return 0 on success
 */
int calibration_at_boresight(
    const int16_t *cal_samples,
    uint16_t num_samples,
    complex_cal_t *cal_table
);

/**
 * @brief Apply calibration corrections to samples
 * @param samples I/Q sample buffer (modified in-place)
 * @param cal_table Calibration table
 * @param num_samples Number of I/Q pairs
 */
void apply_calibration(
    int16_t *samples,
    const complex_cal_t *cal_table,
    uint16_t num_samples
);

/**
 * @brief Save calibration table to memory
 * @param cal_table Calibration data
 * @param offset Flash memory offset
 * @return 0 on success
 */
int calibration_save_flash(const complex_cal_t *cal_table, uint32_t offset);

/**
 * @brief Load calibration table from memory
 * @param cal_table Output buffer
 * @param offset Flash memory offset
 * @return 0 on success
 */
int calibration_load_flash(complex_cal_t *cal_table, uint32_t offset);

// ============================================================================
// Diagnostic & Monitoring API
// ============================================================================

/**
 * @brief Get beamformer internal state
 * @return Pointer to internal state structure
 */
const beamformer_state_t* beamformer_get_state(void);

/**
 * @brief Get beamformer statistics
 * @param out_frame_count Pointer to frame counter
 * @param out_avg_latency_us Average processing latency in microseconds
 * @return 0 on success
 */
int beamformer_get_stats(uint32_t *out_frame_count, uint32_t *out_avg_latency_us);

/**
 * @brief Enable/disable debug logging
 * @param level 0=off, 1=errors, 2=info, 3=debug
 */
void beamformer_set_debug_level(uint8_t level);

/**
 * @brief Calculate and report beam pattern at specific angle
 * @param theta_rad Angle to evaluate
 * @return Gain at angle (in dB)
 */
float beamformer_beam_pattern_at_angle(float theta_rad);

// ============================================================================
// Fixed-Point Math Utilities
// ============================================================================

/**
 * @brief Complex multiplication: out = a * conj(b)
 * @param a First complex number
 * @param b Second complex number
 * @return Result
 */
complex_fp_t fp_cmul_conj(complex_fp_t a, complex_fp_t b);

/**
 * @brief Complex magnitude squared: |a|²
 * @param a Complex number
 * @return Magnitude squared
 */
uint32_t fp_cmagnitude_sq(complex_fp_t a);

/**
 * @brief Convert ADC sample to fixed-point complex
 * @param i_sample I channel (in-phase)
 * @param q_sample Q channel (quadrature)
 * @return Complex fixed-point value
 */
complex_fp_t fp_adc_to_complex(int16_t i_sample, int16_t q_sample);

/**
 * @brief Get sin and cos values from phase lookup table
 * @param phase_idx Index into LUT (0 to PHASE_MAX-1)
 * @param out_cos Output cosine value (fixed-point)
 * @param out_sin Output sine value (fixed-point)
 */
void fp_get_sincos_from_lut(uint16_t phase_idx, int16_t *out_cos, int16_t *out_sin);

// ============================================================================
// Steering Vector Calculation
// ============================================================================

/**
 * @brief Calculate steering vector for given direction
 * @param theta_rad Azimuth angle in radians
 * @param phi_rad Elevation angle in radians (default 0 for ULA)
 * @param out_steering Output vector [NUM_ELEMENTS] complex values
 * @return 0 on success
 */
int steering_vector_calc(float theta_rad, float phi_rad, complex_fp_t *out_steering);

/**
 * @brief Calculate steering vector using pre-computed LUT
 * @param theta_lut_idx Index into phase LUT for theta
 * @param out_steering Output steering vector
 * @return 0 on success
 */
int steering_vector_from_lut(uint16_t theta_lut_idx, complex_fp_t *out_steering);

/**
 * @brief Initialize steering vector lookup table
 * @return 0 on success
 */
int steering_vector_init_lut(void);

// ============================================================================
// System Initialization (called by main)
// ============================================================================

/**
 * @brief Initialize ADC, DMA, and related peripherals
 * @return 0 on success
 */
int adc_dma_init(void);

/**
 * @brief Initialize UART for debug output
 * @param baud_rate Baud rate (typically 115200)
 * @return 0 on success
 */
int uart_debug_init(uint32_t baud_rate);

/**
 * @brief System clock configuration to 180 MHz
 * @return 0 on success
 */
int system_clock_init(void);

// ============================================================================
// UART Debug Output
// ============================================================================

/**
 * @brief Printf-style output over UART
 * @param fmt Format string
 * @return 0 on success
 */
int uart_printf(const char *fmt, ...);

/**
 * @brief Log raw ADC frame for debugging
 * @param frame ADC sample buffer
 * @param num_samples Number of samples
 */
void uart_log_adc_frame(const int16_t *frame, uint16_t num_samples);

/**
 * @brief Report system statistics
 */
void uart_report_stats(void);

#endif // BEAMFORMER_H
