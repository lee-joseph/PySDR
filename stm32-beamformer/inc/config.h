/**
 * @file config.h
 * @brief User-configurable parameters for beamformer
 *
 * Edit this file to customize beamforming behavior for your specific hardware.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// Array Geometry & RF Configuration
// ============================================================================

/** Number of array elements (currently fixed to 4) */
#define NUM_ELEMENTS 4

/** Center frequency in Hz (affects phase calculation) */
#define CENTER_FREQUENCY_HZ 2.8e9  // 2.8 GHz X-band radar

/** Physical spacing between elements in meters */
#define ARRAY_SPACING_M 0.0535  // ~0.5 wavelength @ 2.8 GHz

// Derived from above:
// WAVELENGTH = 3e8 / CENTER_FREQUENCY_HZ = 0.107 m
// Spacing = 0.0535 m = 0.5 λ (ideal for avoiding grating lobes)

// ============================================================================
// ADC Configuration
// ============================================================================

/** ADC sampling rate in Hz */
#define ADC_SAMPLE_RATE_HZ 1000000  // 1 MHz

/** Number of ADC I/Q pairs to collect per frame */
#define SAMPLES_PER_FRAME 256

/** DMA buffer size: SAMPLES_PER_FRAME × 2 (half/full buffers) × NUM_ELEMENTS × 2 (I/Q) */
#define ADC_BUFFER_SIZE (SAMPLES_PER_FRAME * 2 * NUM_ELEMENTS * 2)

/** ADC resolution in bits */
#define ADC_BITS 12

// Derived:
// FRAME_DURATION = SAMPLES_PER_FRAME / ADC_SAMPLE_RATE_HZ = 256 µs
// BEAMFORM_RATE = 1 / FRAME_DURATION ≈ 3.9 kHz (with processing overhead ~2.5 kHz)

// ============================================================================
// Beamforming Algorithm Selection
// ============================================================================

/** Beamforming mode: 0=Conventional, 1=MVDR */
#define BEAMFORMING_MODE 0

// Conventional beamformer:
//   - Weights = steering vector for target direction
//   - Fast, low latency
//   - Less interference suppression

// MVDR beamformer:
//   - Adapts to signal + interference covariance
//   - Better SNR in presence of interference
//   - Requires more computation and samples

// ============================================================================
// MVDR-Specific Parameters (if BEAMFORMING_MODE = 1)
// ============================================================================

/** Number of snapshots (frames) for covariance estimation */
#define MVDR_NUM_SNAPSHOTS 256

/** Diagonal loading for regularization (prevents ill-conditioning) */
#define MVDR_DIAGONAL_LOAD 0.01f

// Larger diagonal_load → more stable but less adaptive
// Typical range: 0.001 to 0.1

// ============================================================================
// Beam Steering & Scanning
// ============================================================================

/** Default steering angle in degrees (boresight = 0) */
#define DEFAULT_STEERING_THETA_DEG 0.0f

/** Enable automatic beam scanning */
#define ENABLE_BEAM_SCAN 0

/** Beam scan range: ±SCAN_RANGE_DEG (if ENABLE_BEAM_SCAN=1) */
#define BEAM_SCAN_RANGE_DEG 60.0f

/** Beam scan step in degrees */
#define BEAM_SCAN_STEP_DEG 1.0f

// ============================================================================
// Phase & Amplitude Calibration
// ============================================================================

/** Enable automatic calibration on startup */
#define ENABLE_AUTO_CALIBRATION 0

/** Flash memory offset for storing calibration data */
#define CALIBRATION_FLASH_OFFSET 0x7C000

/** Calibration reference signal expected power level (dBm) */
#define CALIBRATION_REF_POWER_DBM -10

// ============================================================================
// Output & Debugging
// ============================================================================

/** UART baud rate for debug output */
#define UART_BAUD_RATE 115200

/** Debug verbosity level: 0=off, 1=errors, 2=info, 3=verbose */
#define DEBUG_LEVEL 2

/** Enable profiling (measures CPU time per frame) */
#define ENABLE_PROFILING 1

/** Enable logging of raw ADC samples (warning: high UART bandwidth) */
#define LOG_RAW_ADC_SAMPLES 0

/** Log beamformer output every N frames (0 = every frame) */
#define LOG_OUTPUT_DECIMATION 10

// ============================================================================
// Performance Tuning
// ============================================================================

/** Fixed-point precision bits for arithmetic */
#define FP_BITS 14

/** Phase lookup table resolution in bits */
#define PHASE_BITS 12

/** Enable ARM SIMD optimizations (if supported) */
#define USE_ARM_INTRINSICS 1

/** Use inline assembly for critical loops */
#define USE_INLINE_ASM 0

// ============================================================================
// Calibration & Correction
// ============================================================================

/** Per-element phase offset calibration (in degrees, relative to element 0) */
// Set to 0 if using automatic calibration
#define PHASE_OFFSET_ELEMENT_0 0.0f
#define PHASE_OFFSET_ELEMENT_1 0.0f
#define PHASE_OFFSET_ELEMENT_2 0.0f
#define PHASE_OFFSET_ELEMENT_3 0.0f

/** Per-element amplitude scaling (linear, 1.0 = no scaling) */
#define AMP_SCALE_ELEMENT_0 1.0f
#define AMP_SCALE_ELEMENT_1 1.0f
#define AMP_SCALE_ELEMENT_2 1.0f
#define AMP_SCALE_ELEMENT_3 1.0f

// ============================================================================
// Array Tapering (Sidelobe Control)
// ============================================================================

/** Taper type: 0=Uniform, 1=Hanning, 2=Hamming, 3=Blackman */
#define TAPER_TYPE 0

// Taper trade-offs:
// Uniform (0):   Lowest mainlobe width, highest sidelobes (-13 dB)
// Hanning (1):   Moderate sidelobes (-43 dB)
// Hamming (2):   Good balance (-53 dB)
// Blackman (3):  Lowest sidelobes (-58 dB), widest mainlobe

// ============================================================================
// Runtime Assertions & Safety
// ============================================================================

/** Enable bounds checking (costs CPU cycles) */
#define ENABLE_BOUNDS_CHECK 1

/** Maximum allowed steering angle in degrees (0-180) */
#define MAX_STEERING_ANGLE_DEG 90.0f

/** Assert on covariance matrix ill-conditioning */
#define ASSERT_ON_ILL_CONDITIONING 1

// ============================================================================
// Memory Layout (for reference - do NOT edit)
// ============================================================================

// RAM usage estimate:
// - ADC buffer:        4 KB (double-buffered circular)
// - Beamformer state:  ~1 KB
// - Covariance (MVDR): 2 KB (16x16 complex matrix)
// - Stack:             ~8 KB
// TOTAL:               ~15 KB (with ~150 KB available on STM32F446RE)

// Flash usage estimate:
// - Code:              ~25 KB
// - Phase LUT:         24 KB
// - Calibration:       1 KB
// TOTAL:               ~50 KB (with ~256 KB available)

#endif // CONFIG_H
