# STM32F446RE Beamforming: Hardware Setup & Deployment Guide

## Quick Start

### What You Need
1. **STM32F446RE Nucleo Board** (ST official or clone)
2. **4-Element RF Frontend** (RF down-converter to I/Q baseband or low IF)
3. **4-Element Linear Antenna Array** at microwave frequency
4. **ST-Link Debugger** (included with Nucleo)
5. **STM32CubeIDE** (free, Eclipse-based)
6. **USB Cable** for programming and UART debug

### Project Files Provided
```
├── stm32_beamforming_guide.md          # Complete technical reference
├── stm32f446re_beamformer_complete.c   # Ready-to-compile example
├── beamformer.h                         # Header/API definitions
└── This file (setup & deployment)
```

---

## Hardware Setup

### RF Frontend Architecture

```
Antenna Array (4x)
    ↓
LNA (Low Noise Amplifier)
    ↓
Band-pass Filter (passband centered on RF)
    ↓
Mixer/Down-Converter
    ├─ Local Oscillator (RF/2 or similar)
    └─ Output: baseband or low IF suitable for ADC sampling
    ↓
I/Q Demodulator
    ├─ I-channel (In-phase) → STM32 ADC input
    └─ Q-channel (Quadrature) → STM32 ADC input
    ↓
[Repeat for elements 2-4]
    ↓
Low-Pass Filter (removes images)
    ↓
ADC Buffers (optional op-amp, ~1V peak input for 12-bit ADC)
```

### Pin Assignments on STM32F446RE

The STM32F446RE has three internal ADC peripherals, not four. This example uses
ADC1 in scan mode across PA0-PA7, giving eight deterministic but not truly
simultaneous I/Q samples. For tight phase-coherence requirements, use an
external simultaneous-sampling ADC or calibrate the ADC scan skew.

| Signal | Channel | Pin | Notes |
|--------|---------|-----|-------|
| Element 0 - I | ADC1_CH0 | PA0 | Can use external mux for RF switching |
| Element 0 - Q | ADC1_CH1 | PA1 | |
| Element 1 - I | ADC1_CH2 | PA2 | |
| Element 1 - Q | ADC1_CH3 | PA3 | |
| Element 2 - I | ADC1_CH4 | PA4 | |
| Element 2 - Q | ADC1_CH5 | PA5 | |
| Element 3 - I | ADC1_CH6 | PA6 | |
| Element 3 - Q | ADC1_CH7 | PA7 | |
| Debug UART TX | USART3 | PB10 | Connect to FTDI/CP2102 for monitoring |
| Debug UART RX | USART3 | PB11 | Optional, for remote control |
| SPI1 SCK | SPI1 | PA5 | Optional; conflicts with Element 2 Q in this pinout |
| SPI1 MOSI | SPI1 | PA7 | Optional; conflicts with Element 3 Q in this pinout |
| SPI1 SS | GPIO | PB6 | Optional: Chip select for DAC/phase shifter |

### ADC Input Range

**Critical**: STM32F446RE ADC input is **0V to VREF+ (3.3V)**

Your IF signal must be **AC-coupled and biased** to mid-rail:

```
IF Signal (±500 mV)
    ↓
AC Coupling Capacitor (1 µF)
    ↓
Bias Network (1M / 1M voltage divider to 1.65V)
    ↓
Buffer Op-Amp (TL072 or similar, unity gain)
    ↓
ADC Input (0.65V to 2.65V swing)
```

Example circuit for one channel:
```
IF IN ──┬──┬── 1µF Cap ──┬── [100k to 1.65V bias] ──┬── PA0 (ADC)
        │  │             │                           │
        │ [1µ to GND]    [100k to GND]              [10k to GND]
```

---

## STM32CubeIDE Setup

### 1. Create New Project

```
File → New → STM32 Project
```

Select: **STM32F446RE** (from "MCU Selector")

### 2. Configure Clock

Under "Clocks" tab in CubeMX:
- **System Clock**: 180 MHz (uses internal HSI + PLL)
- **APB1**: 45 MHz (divide by 4)
- **APB2**: 90 MHz (divide by 2)
- **ADC Clock**: 22.5 MHz (APB2 / 4)

### 3. Configure ADC1

**Pinout & Configuration**:
- ADC1 → Regular Conversion Mode
- ADC1 → Continuous Conversion
- ADC1 → Enable DMA
- ADC1 → Scan Mode: Enabled

**Channels**:
- Rank 1: IN0 (PA0)
- Rank 2: IN1 (PA1)
- Rank 3: IN2 (PA2)
- Rank 4: IN3 (PA3)
- Rank 5: IN4 (PA4)
- Rank 6: IN5 (PA5)
- Rank 7: IN6 (PA6)
- Rank 8: IN7 (PA7)

**Sampling Time**: 28.5 cycles (balance speed/accuracy)

**DMA**:
- DMA2 Stream 0 (for ADC1)
- Mode: Circular
- Data Width: 16-bit halfword
- Increment: Memory
- Priority: Very High

### 4. Configure USART3 (Debug)

- Mode: TX only (unless you want remote steering)
- Baud Rate: 115200
- Data Bits: 8
- Stop Bits: 1
- Parity: None

### 5. Configure NVIC

Enable interrupts:
- DMA2 Stream 0 global interrupt: **Priority 2**
- USART3 global interrupt (optional): Priority 3

### 6. Generate Code

```
Project → Generate Code
```

This creates:
- `Src/main.c` (empty, we'll replace)
- `Src/stm32f4xx_it.c` (interrupt handlers)
- `Inc/stm32f4xx_hal_conf.h` (HAL configuration)

---

## Integrating the Beamformer Code

### Step 1: Replace main.c

Copy `stm32f446re_beamformer_complete.c` content into `Src/main.c`.

### Step 2: Add Header

Copy `beamformer.h` to `Inc/beamformer.h`.

### Step 3: Add Interrupt Handler

In `Src/stm32f4xx_it.c`, add:

```c
extern volatile uint32_t dma_half_complete;
extern volatile uint32_t dma_full_complete;

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

### Step 4: Build & Compile

```
Project → Build All
```

Fix any linker warnings (usually ignored).

### Step 5: Program Board

```
Run → Debug  (F11 to debug, or Shift+F11 for release run)
```

---

## Testing & Validation

### Test 1: Basic Operation Check

1. Connect USB to Nucleo board
2. Open Serial Monitor (115200 baud):
   - **CubeIDE**: Window → Show View → Serial Console
   - **Or use**: PuTTY/Minicom with `/dev/ttyUSBX`

3. Should see:
   ```
   === STM32F446RE Beamformer Starting ===
   Frame: 100, Power: 45 dB, Quality: 95%
   Frame: 200, Power: 46 dB, Quality: 95%
   ...
   ```

### Test 2: ADC Data Verification

Add debug output to check raw ADC values:

```c
// In beamformer_process_frame():
static uint32_t debug_counter = 0;
if (debug_counter++ % 10000 == 0) {
    printf("Raw ADC: I0=%d Q0=%d I1=%d Q1=%d\r\n",
           frame_buffer[0], frame_buffer[1],
           frame_buffer[2], frame_buffer[3]);
}
```

**Expected**: Should vary sinusoidally if IF signal is present, ±500 counts.

### Test 3: Beamforming Validation

Connect a **known signal source** (function generator + mixer) at specific angle:

1. **Input**: 10 MHz IF signal, -10 dBm
2. **Expected**: Power output increases when beam steered to signal direction
3. **Check**: Monitor power via UART for different steering angles

### Test 4: Phase Coherence Check

Use 4 **phase-locked test signals** via RF splitter:

```
SMA RF Input
    ↓
Wilkinson Splitter (4-way)
    ├─→ Element 0 RF Frontend
    ├─→ Element 1 RF Frontend
    ├─→ Element 2 RF Frontend
    └─→ Element 3 RF Frontend
```

If all signals are in-phase (from same source):
- Peak power when θ = 0° (boresight)
- Power ∝ 20*log10(4) = 12 dB relative to single element
- Null at ±90°

---

## Real-World Calibration

### Phase Calibration

Errors from RF components, cable lengths, and ADC mismatches require calibration:

```c
// For each element n:
// phase_offset[n] = actual_phase[n] - expected_phase[n]
// Apply correction:
// corrected_phase[n] = steering_phase[n] - phase_offset[n]
```

**Procedure**:
1. Point array at **boresight** with known signal
2. Compute phase of each element's received signal
3. Store offsets in flash memory
4. Apply in `beamformer_update_weights()`:

```c
// Corrected weight:
phase_idx_corrected = (phase_idx - phase_offset_lut[n] + PHASE_MAX) % PHASE_MAX;
```

### Amplitude Calibration

Similarly correct for gain differences (LNA, cable, mixer):

```c
// In fp_cmul_conj():
weighted.real = ((amplitude_correction[ch] * received.real * weight.real) >> (FP_BITS*2));
```

---

## Performance Metrics

### Memory Usage
- **RAM**: ~40 KB (DMA buffer + state)
- **Flash**: ~30 KB (code + LUT)
- **Remaining**: ~150 KB for future expansion

### Computation Time
- **Per sample frame**: ~50 CPU cycles (4 complex channels × MAC)
- **Per 256-frame beamforming block**: ~70-150 µs, depending on compiler settings
- **Effective beamform update rate**: ~488 Hz with a 1 MHz aggregate ADC scan rate

### Latency
- **From RF to output**: ~2.2 ms with the ADC1 scan example
- **Jitter**: <10 µs (DMA-driven, low jitter)

### Power Consumption
- **Idle** (no processing): ~80 mA
- **Full beamforming**: ~120 mA
- **Total @ 5V USB**: ~600 mW

---

## Optimization & Extensions

### 1. Adaptive Beamforming (MVDR)

For interference nulling, implement:

```c
// Covariance matrix: R = E[r·r†]
// MVDR weights: w = R⁻¹·s / (s†·R⁻¹·s)
```

Requires matrix inversion (expensive on STM32). Consider:
- Use CMSIS-DSP `arm_mat_inverse_f32()` (requires FPU)
- Pre-compute for limited angle set
- Or use iterative method (Levinson recursion)

### 2. Multi-Beam Capability

Compute multiple beams simultaneously:

```c
// Separate weight sets for different angles
complex_fp_t weights_beam1[NUM_ELEMENTS];
complex_fp_t weights_beam2[NUM_ELEMENTS];
// Process in parallel: y1 = sum(w1[n]*r[n]), y2 = sum(w2[n]*r[n])
```

Cost: O(N*M) where M = number of beams.

### 3. DOA Estimation (MUSIC)

Computationally expensive; consider:
- Pre-compute noise subspace eigenvectors offline
- Do MUSIC sweep in background task (lower priority)
- Output peak angles via UART

### 4. Analog Phase Shifters

Instead of DSP beamforming, use hardware phase shifters (AD5201, HMC7992):

```c
// Update SPI registers to set analog phase
for (int n = 0; n < NUM_ELEMENTS; n++) {
    uint8_t phase_code = steering_phase(theta, n, d_over_lambda) >> 4;  // 0-255
    SPI_Write(PHASE_SHIFTER_CS[n], phase_code);
}
```

Benefits:
- Avoids digital processing (lower power)
- Lower latency
- BUT: less flexibility (discrete phase steps)

---

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| No ADC data | ADC not triggered | Check ADC_CR2_SWSTART, verify DMA enabled |
| DMA not running | Clock not enabled | Ensure `RCC->AHB1ENR \|= RCC_AHB1ENR_DMA2EN` |
| Wrong beamform direction | Phase calculation error | Check `steering_phase()` math, verify angle units (radians!) |
| Power always low | ADC saturation or clipping | Check bias network, reduce IF signal amplitude |
| Crashes after startup | Stack overflow | Increase SRAM allocation in linker script |
| UART no output | USART clock not enabled | `RCC->APB1ENR \|= RCC_APB1ENR_USART3EN` |

---

## Next: Real Radar Application

Once basic beamforming works:

1. **Add pulse radar functionality**:
   - Transmit pulse via GPIO + RF switch
   - Measure receive time delay (range)
   - Combine with beamforming (range + angle)

2. **Implement target tracking**:
   - Kalman filter for angle/range state
   - Predict next beam steering position
   - Reduce computation by tracking subset of angles

3. **Deploy on production hardware**:
   - Design custom PCB (RF frontend + antenna array)
   - Use commercial RF ICs (e.g., Analog Devices ADF4159 PLL for transmit)
   - Implement proper thermal management

---

## References

- PySDR Textbook: `/Users/joseph/PySDR/content/2d_beamforming.rst`
- STM32F446RE product page and datasheet: https://www.st.com/en/microcontrollers-microprocessors/stm32f446re.html
- CMSIS-DSP Documentation: https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
- Beamforming Fundamentals: "Phased Array Antenna Handbook" by R. J. Mailloux
