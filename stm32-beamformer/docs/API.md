# STM32 Beamformer API Reference

## Core Functions

### Initialization

```c
int beamformer_init(const beamformer_config_t *cfg);
```
Initialize the beamformer with given configuration. Must be called before processing.

**Parameters:**
- `cfg`: Configuration structure with frequency, array spacing, etc.

**Returns:** 0 on success, -1 on error

**Example:**
```c
beamformer_config_t cfg = {
    .frequency_hz = 2.8e9,
    .array_spacing_m = 0.0535,
    .num_elements = 4,
    .beamforming_mode = BEAMFORMER_CONVENTIONAL,
    .target_theta_rad = 0.0,
};
beamformer_init(&cfg);
```

### Processing

```c
beamform_result_t beamformer_process_frame(
    const int16_t *samples,
    uint16_t num_samples
);
```
Process one frame of ADC samples.

**Parameters:**
- `samples`: I/Q sample buffer (interleaved: I0, Q0, I1, Q1, I2, Q2, I3, Q3)
- `num_samples`: Number of I/Q pairs

**Returns:** Result structure with power and metrics

### Beam Steering

```c
void beamformer_steer(float theta_rad);
```
Dynamically update beam steering angle.

**Parameters:**
- `theta_rad`: Target azimuth angle in radians (-π to π)

---

## Calibration Functions

### Phase Calibration

```c
int calibration_at_boresight(
    const int16_t *cal_samples,
    uint16_t num_samples,
    complex_cal_t *cal_table
);
```
Compute calibration offsets using a known source at boresight.

**Parameters:**
- `cal_samples`: Calibration signal samples
- `num_samples`: Number of samples to process
- `cal_table`: Output calibration table [NUM_ELEMENTS]

**Returns:** 0 on success

---

## Fixed-Point Math

### Complex Operations

```c
complex_fp_t fp_cmul_conj(complex_fp_t a, complex_fp_t b);
```
Multiply a × conj(b) in fixed-point.

```c
uint32_t fp_cmagnitude_sq(complex_fp_t a);
```
Calculate |a|².

---

## Configuration

Edit `inc/config.h` to customize:

```c
#define CENTER_FREQUENCY_HZ 2.8e9
#define ARRAY_SPACING_M 0.0535
#define BEAMFORMING_MODE 0  // 0=Conventional, 1=MVDR
#define DEBUG_LEVEL 2
```

---

## Performance Notes

- **Beamform Rate:** ~2.5 kHz (with processing overhead)
- **Latency:** <300 µs end-to-end
- **RAM:** ~40 KB used / 192 KB available
- **Flash:** ~50 KB used / 256 KB available

---

## Troubleshooting

### ADC Data is Noise
- Check RF signal level (should be ±500 mV at ADC input)
- Verify 1.65V bias network
- Check AC coupling capacitor connections

### Beamformer Output Doesn't Change
- Enable DEBUG_LEVEL=3 in config.h
- Verify array spacing in configuration
- Check steering_vector.c phase calculations

See `TROUBLESHOOTING.md` for more issues and solutions.
