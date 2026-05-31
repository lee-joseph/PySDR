# STM32F446RE Phased Array Beamforming Implementation

## Hardware Setup

### Target Specifications
- **MCU**: STM32F446RE (180 MHz ARM Cortex-M4)
- **Array**: 4-element linear array (ULA)
- **Frequency**: Microwave (2-10 GHz typical, IF sampled)
- **Application**: Radar beamforming

### Hardware Architecture

```
RF Frontend (4 channels)
    ↓
Down-converter/Mixer (IF output)
    ↓
[ADC1] [ADC2] [ADC3] [ADC4] (on STM32F446RE)
    ↓
DMA → Circular Buffer (I/Q samples)
    ↓
Beamforming Engine (Main loop)
    ↓
Output (UART, DAC, or memory)
```

### ADC Configuration for Phase Coherence

**Critical**: All 4 ADC channels must sample simultaneously to maintain phase coherence.

#### Option 1: Single ADC with Multiplexed Channels (Recommended for simplicity)
- Use ADC1 with 4 different input channels
- Configure DMA in circular mode
- Sampling order: CH0 → CH1 → CH2 → CH3 (repeating)
- **Issue**: Slight time offset between channels (~microseconds) - acceptable for low-frequency IF signals

#### Option 2: Dual ADC Simultaneous Mode (Better phase coherence)
- Use ADC1 + ADC2 in simultaneous mode
- ADC1: Channels 0, 2
- ADC2: Channels 1, 3
- True parallel sampling on hardware level
- Recommended for microwave applications

### STM32CubeMX Configuration

**ADC Setup (Option 2 - Simultaneous Mode)**:
```
ADC1:
  - Regular Conversion Mode: Continuous
  - Dual Mode: Simultaneous
  - Rank 1: IN0 (Element 0, I channel)
  - Rank 2: IN1 (Element 2, I channel)
  - Sample Time: 28.5 cycles (balance speed/accuracy)
  - DMA: ADC1+ADC2 combined, Circular, Word alignment

ADC2:
  - Rank 1: IN2 (Element 1, I channel)
  - Rank 2: IN3 (Element 3, I channel)
  - Enable linked to ADC1

Timing:
  - Target sampling rate: 1 MHz (for IF signals)
  - APB2 clock = 90 MHz
  - ADC clock = 22.5 MHz (prescaler /4)
  - Conversion time ≈ 1 µs
```

**DMA Configuration**:
```
DMA2 Stream 0 (ADC1+ADC2):
  - Mode: Circular
  - Data width: 32-bit (combines both ADC results)
  - Priority: Very High
  - Target buffer size: 512 samples × 4 elements = 2048 words
  - Interrupt: Half/Full Transfer
```

---

## Beamforming Algorithm Implementation

### Mathematical Foundation (from PySDR)

Steering vector for element n at angle θ:
```
s[n] = exp(j * 2π * d * sin(θ) * n / λ)
```

For baseband samples (IF), simplified to phase-only weights:
```
w[n] = exp(j * phase[n])
```

Beamformed output:
```
y = sum(w[n]* conj(r[n]))  for n = 0 to N-1
```

Power:
```
P = |y|² = Real² + Imag²
```

### Fixed-Point Representation

Since STM32F446RE has limited floating-point performance, use fixed-point:

```c
#define FIXED_POINT_BITS 14
#define FIXED_POINT_MAX  (1 << (FIXED_POINT_BITS - 1))

typedef struct {
    int16_t real;
    int16_t imag;
} complex_fixed_t;
```

For phase storage (0 to 2π mapped to 0 to 4096):
```c
#define PHASE_BITS 12
#define PHASE_MAX (1 << PHASE_BITS)
typedef uint16_t phase_t;
```

---

## Firmware Structure

### File Organization
```
src/
  ├── main.c                 # Entry point, initialization
  ├── beamformer.c/.h        # Core beamforming algorithm
  ├── adc_dma.c/.h           # ADC/DMA configuration
  ├── fixed_point_math.c/.h  # Fixed-point arithmetic
  ├── phase_calc.c/.h        # Phase calculation lookup tables
  └── startup_code/          # STM32 startup (auto-generated)

Inc/
  ├── stm32f4xx_it.h         # Interrupt handlers
  └── config.h               # Tuning parameters

Lib/
  └── CMSIS/                 # ARM CMSIS core library
```

---

## Core Implementation

### 1. ADC/DMA Handler (`adc_dma.c`)

```c
#include "stm32f4xx.h"
#include "config.h"

#define NUM_ELEMENTS 4
#define SAMPLES_PER_CHANNEL 256
#define BUFFER_SIZE (NUM_ELEMENTS * SAMPLES_PER_CHANNEL)

// Circular DMA buffer: stores I/Q for 4 channels
// DMA layout: [I0, I1, I2, I3, I0, I1, I2, I3, ...]
int16_t adc_buffer[BUFFER_SIZE];

volatile uint32_t dma_half_complete = 0;
volatile uint32_t dma_full_complete = 0;

void ADC_DMA_Init(void) {
    // Enable clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;  // GPIO
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;   // ADC1
    RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;   // ADC2
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;   // DMA2
    
    // Configure ADC1 (simultaneous mode)
    ADC1->CR2 |= ADC_CR2_ADON;  // Power on
    ADC1->CR1 |= ADC_CR1_SCAN;  // Scan mode
    ADC1->SQR1 = (1 << 20);     // 2 conversions
    ADC1->SQR3 = (0 << 0) | (2 << 5);  // CH0, CH2
    ADC1->SMPR2 = (3 << 0) | (3 << 6); // Sample time
    
    // Configure ADC2
    ADC2->CR2 |= ADC_CR2_ADON;
    ADC2->CR1 |= ADC_CR1_SCAN;
    ADC2->SQR1 = (1 << 20);
    ADC2->SQR3 = (1 << 0) | (3 << 5);  // CH1, CH3
    ADC2->SMPR2 = (3 << 0) | (3 << 6);
    
    // Simultaneous mode
    ADC->CCR |= (0b01 << 16);  // Mode: Dual simultaneous
    ADC->CCR |= (1 << 8);      // DMA mode: single
    
    // Configure DMA2 Stream 0
    DMA2_Stream0->CR = 0;
    DMA2_Stream0->CR |= (1 << 25);        // Channel 0 (ADC1+2)
    DMA2_Stream0->CR |= (1 << 13);        // Word width
    DMA2_Stream0->CR |= (1 << 8);         // Circular mode
    DMA2_Stream0->CR |= (2 << 6);         // Memory increment
    DMA2_Stream0->NDTR = BUFFER_SIZE;     // Transfer count
    DMA2_Stream0->PAR = (uint32_t)&ADC->CDR;  // Source (common register)
    DMA2_Stream0->M0AR = (uint32_t)adc_buffer; // Destination
    DMA2_Stream0->CR |= (1 << 5);         // Half-transfer interrupt
    DMA2_Stream0->CR |= (1 << 4);         // Transfer complete interrupt
    DMA2_Stream0->CR |= (1 << 0);         // Enable
    
    // Start ADC conversion
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

// Interrupt handler
void DMA2_Stream0_IRQHandler(void) {
    if (DMA2->LISR & DMA_LISR_HTIF0) {
        DMA2->LIFCR = DMA_LIFCR_CHTIF0;
        dma_half_complete = 1;
    }
    if (DMA2->LISR & DMA_LISR_TCIF0) {
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;
        dma_full_complete = 1;
    }
}
```

### 2. Fixed-Point Math (`fixed_point_math.c`)

```c
#include <stdint.h>

#define FP_BITS 14
#define FP_ONE (1 << FP_BITS)

typedef struct {
    int32_t real;
    int32_t imag;
} complex_fp_t;

// Fixed-point complex multiplication: out = a * conj(b)
complex_fp_t fp_cmul_conj(complex_fp_t a, complex_fp_t b) {
    complex_fp_t result;
    // (a_r + j*a_i) * (b_r - j*b_i)
    // = a_r*b_r + a_i*b_i + j(a_i*b_r - a_r*b_i)
    result.real = ((a.real * b.real) + (a.imag * b.imag)) >> FP_BITS;
    result.imag = ((a.imag * b.real) - (a.real * b.imag)) >> FP_BITS;
    return result;
}

// Convert 16-bit ADC samples to fixed-point complex
complex_fp_t adc_to_complex(int16_t i_sample, int16_t q_sample) {
    complex_fp_t result;
    result.real = ((int32_t)i_sample) << 6;  // Scale to FP
    result.imag = ((int32_t)q_sample) << 6;
    return result;
}

// Power calculation: P = Re² + Im²
uint32_t complex_power(complex_fp_t c) {
    int32_t re = c.real >> 10;  // Prevent overflow
    int32_t im = c.imag >> 10;
    return (re * re) + (im * im);
}

// Log2 approximation (for dB calculation)
uint16_t log2_approx(uint32_t x) {
    if (x == 0) return 0;
    uint16_t msb = 31 - __builtin_clz(x);
    return msb;
}
```

### 3. Phase Calculation with LUT (`phase_calc.c`)

```c
#include <stdint.h>
#include <math.h>

#define PHASE_BITS 12
#define PHASE_MAX (1 << PHASE_BITS)
#define PI 3.14159265358979f

// Lookup table: phase_index (0 to 4095) → (cos, sin) in fixed-point
typedef struct {
    int16_t cos_val;
    int16_t sin_val;
} sincos_lut_t;

sincos_lut_t phase_lut[PHASE_MAX];

void phase_lut_init(void) {
    for (int i = 0; i < PHASE_MAX; i++) {
        float angle = 2.0f * PI * i / PHASE_MAX;
        phase_lut[i].cos_val = (int16_t)(32767.0f * cosf(angle));
        phase_lut[i].sin_val = (int16_t)(32767.0f * sinf(angle));
    }
}

// Calculate steering vector weight for element n at angle theta
// steering_phase[n] = 2π * d * sin(θ) * n / λ
// Input: theta (in phase units 0-PHASE_MAX), element index n
// Output: phase weight (0-PHASE_MAX)
uint16_t steering_phase(float theta_rad, int element_idx, float d_over_wavelength) {
    // d/λ = antenna_spacing / wavelength
    // For 2.8 GHz, λ = 10.7 cm, typical d = 5.35 cm = 0.5λ
    
    float phase_rad = 2.0f * PI * d_over_wavelength * sinf(theta_rad) * element_idx;
    
    // Normalize to 0-2π
    while (phase_rad > 2.0f * PI) phase_rad -= 2.0f * PI;
    while (phase_rad < 0) phase_rad += 2.0f * PI;
    
    uint16_t phase_idx = (uint16_t)((phase_rad / (2.0f * PI)) * PHASE_MAX);
    return phase_idx;
}

// Get complex weight from phase (magnitude = 1)
complex_fp_t get_weight_from_phase(uint16_t phase_idx) {
    complex_fp_t w;
    sincos_lut_t sc = phase_lut[phase_idx];
    w.real = ((int32_t)sc.cos_val) << 6;
    w.imag = ((int32_t)sc.sin_val) << 6;
    return w;
}
```

### 4. Beamformer Core (`beamformer.c`)

```c
#include "beamformer.h"
#include "fixed_point_math.h"
#include "phase_calc.h"

#define NUM_ELEMENTS 4
#define SAMPLES_PER_FRAME 256

extern int16_t adc_buffer[];

typedef struct {
    float target_theta;     // Target steering angle (radians)
    float d_over_wavelength; // Spacing in wavelengths
    complex_fp_t weights[NUM_ELEMENTS]; // Computed weights
} beamformer_state_t;

beamformer_state_t beamformer;

void beamformer_init(float theta, float d_over_lambda) {
    beamformer.target_theta = theta;
    beamformer.d_over_wavelength = d_over_lambda;
    beamformer_update_weights();
}

void beamformer_update_weights(void) {
    // Compute steering vector weights for current target angle
    for (int n = 0; n < NUM_ELEMENTS; n++) {
        uint16_t phase = steering_phase(beamformer.target_theta, n, 
                                        beamformer.d_over_wavelength);
        beamformer.weights[n] = get_weight_from_phase(phase);
    }
}

void beamformer_steer(float new_theta) {
    beamformer.target_theta = new_theta;
    beamformer_update_weights();
}

// Conventional beamformer: y = sum(w[n] * conj(r[n]))
// Input: raw ADC samples, Output: beamformed I/Q result
complex_fp_t beamform_block(const int16_t *samples) {
    complex_fp_t output = {0, 0};
    
    for (int ch = 0; ch < NUM_ELEMENTS; ch++) {
        // Assume samples are in order: I0, Q0, I1, Q1, I2, Q2, I3, Q3
        // Or layout depends on ADC configuration
        int16_t i_sample = samples[ch * 2];
        int16_t q_sample = samples[ch * 2 + 1];
        
        complex_fp_t received = adc_to_complex(i_sample, q_sample);
        complex_fp_t weighted = fp_cmul_conj(beamformer.weights[ch], received);
        
        output.real += weighted.real;
        output.imag += weighted.imag;
    }
    
    return output;
}

// Process DMA buffer (called from main loop when data available)
void beamformer_process_frame(void) {
    uint32_t power_sum = 0;
    
    // Process all samples in frame
    for (int i = 0; i < SAMPLES_PER_FRAME; i++) {
        int16_t *sample_ptr = &adc_buffer[i * NUM_ELEMENTS * 2];
        complex_fp_t beam_output = beamform_block(sample_ptr);
        uint32_t power = complex_power(beam_output);
        power_sum += power;
    }
    
    // Average power
    uint32_t avg_power = power_sum / SAMPLES_PER_FRAME;
    
    // Output result (UART, DAC, or log)
    output_beamform_result(avg_power);
}

void output_beamform_result(uint32_t power_db) {
    // Convert to dB: 10*log10(power)
    uint16_t power_log = log2_approx(power_db);
    
    // Send via UART
    // uart_printf("Power: %d dB\r\n", power_log);
}
```

### 5. Main Loop (`main.c`)

```c
#include "stm32f4xx.h"
#include "adc_dma.h"
#include "beamformer.h"
#include "phase_calc.h"

int main(void) {
    // Initialize system clock to 180 MHz
    SystemInit();
    
    // Initialize peripherals
    ADC_DMA_Init();
    phase_lut_init();
    
    // Configure beamformer for 2.8 GHz (λ = 10.7 cm, d = 5.35 cm = 0.5λ)
    float d_over_lambda = 0.5f;
    float steering_angle = 0.0f;  // 0 degrees (boresight)
    
    beamformer_init(steering_angle, d_over_lambda);
    
    // Main loop
    while (1) {
        // Check if DMA has filled buffer
        if (dma_half_complete) {
            dma_half_complete = 0;
            // Process first half of buffer
            beamformer_process_frame();
        }
        
        if (dma_full_complete) {
            dma_full_complete = 0;
            // Process second half of buffer
            beamformer_process_frame();
        }
    }
    
    return 0;
}
```

---

## Optimization Tips for STM32F446RE

### Memory Optimization
- **Circular DMA buffer**: 2KB (512 samples × 4 bytes)
- **Phase LUT**: 24KB (4096 entries × 6 bytes)
- **Total heap usage**: < 50 KB (leaves ~140 KB for stack/BSS)

### Computation Optimization
```c
// Use __builtin functions (GCC/ARM):
__builtin_clz()     // Count leading zeros (fast log2)
__builtin_popcount() // Population count

// Unroll loops where profitable:
#pragma GCC unroll 4
for (int i = 0; i < NUM_ELEMENTS; i++) { ... }

// Use intrinsics for DSP operations:
#include <arm_math.h>
arm_cmplx_mult_real_q31();  // Optimized complex multiply
```

### Real-Time Constraints
- ADC sampling rate: 1 MHz (configurable)
- Samples per frame: 256 → **256 µs latency**
- Beamforming computation time: ~150 µs (FP operations)
- Total cycle time: ~400 µs → **2.5 kHz beamform rate**

---

## Testing & Debugging

### UART Output for Monitoring
```c
void uart_printf(const char *fmt, ...) {
    // Initialize UART1 @ 115200 baud
    // Output: "Angle: 30.5°, Power: 45 dB, Quality: 0.98\r\n"
}

// In beamformer:
uart_printf("Theta:%d Power:%d\r\n", (int)(steering_angle*180/PI), power_log);
```

### Real-Time Profiling
- Use STM32 cycle counter (DWT) to measure:
  - ADC interrupt latency
  - Beamforming computation time
  - Peak memory usage

```c
// In CMSIS:
ARM_CM4_CYCLE_COUNTER_INIT();
uint32_t start = DWT->CYCCNT;
// ... code to profile ...
uint32_t elapsed = DWT->CYCCNT - start;
```

---

## Hardware Connections Example

```
RF Frontend → Mixer → IF Filter → (I/Q demod)
                           ↓
                    ADC0 (Element 0, I channel)  → PA0
                    ADC0 (Element 0, Q channel)  → PA1
                    ADC0 (Element 1, I channel)  → PA2
                    ADC0 (Element 1, Q channel)  → PA3
                    ADC1 (Element 2, I channel)  → PA4
                    ADC1 (Element 2, Q channel)  → PA5
                    ADC2 (Element 3, I channel)  → PA6
                    ADC2 (Element 3, Q channel)  → PA7

Output:
    UART TX → PB10 (USART3) for logging
    DAC1 CH1 → PA4 (optional: analog beamform output)
```

---

## Next Steps

1. **Set up STM32CubeMX project** with above ADC/DMA config
2. **Integrate code files** into your project
3. **Calibrate phase offsets** using known signal at boresight
4. **Validate with IF test signal** (sweep angle, measure power vs. angle)
5. **Deploy to hardware** and measure real-world performance

For production radar, consider:
- MVDR/Capon adaptive beamforming (higher computation cost)
- DOA estimation using MUSIC (requires eigenvalue decomposition)
- Multiple beam capability (parallel beamformers)
