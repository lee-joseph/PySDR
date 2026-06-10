# Project Setup Guide

## Directory Structure

```
stm32-beamformer/
├── src/                    # Implementation
│   ├── main.c             # Entry point
│   ├── beamformer.c       # Core algorithms
│   ├── steering_vector.c  # Phase calculations
│   ├── adc_dma.c          # ADC/DMA config
│   ├── uart_debug.c       # Serial logging
│   ├── calibration.c      # Phase calibration
│   ├── fixed_point_math.c # Math utilities
│   └── system_init.c      # Clock and GPIO
├── inc/                   # Headers
│   ├── beamformer.h
│   ├── steering_vector.h
│   ├── adc_dma.h
│   ├── uart_debug.h
│   ├── calibration.h
│   ├── fixed_point_math.h
│   ├── system_init.h
│   ├── config-stm32-beamformer.h      # Configuration
│   └── beamformer-project-API.h       # Public API
├── docs/
│   ├── API.md             # Function reference
│   ├── TROUBLESHOOTING.md # Debug guide
│   ├── BUILD.md           # Build instructions
│   └── SETUP.md           # This file
├── tools/
│   ├── generate_phase_lut.py    # LUT generator
│   ├── parse_uart_logs.py       # Serial monitor
│   └── validate_beamformer.py   # Simulation/validation
├── CMakeLists.txt         # Build configuration
└── README.md              # Project overview
```

## Quick Start

### 1. Clone and Navigate

```bash
cd stm32-beamformer
```

### 2. Configure Hardware

Edit `inc/config-stm32-beamformer.h`:

```c
#define CENTER_FREQUENCY_HZ 2.8e9      /* Your RF frequency */
#define ARRAY_SPACING_M 0.0535         /* 0.5λ @ 2.8 GHz */
#define BEAMFORMING_MODE 0             /* Conventional mode */
#define NUM_ELEMENTS 4
#define SAMPLES_PER_FRAME 256
```

### 3. Generate Lookup Tables

```bash
python3 tools/generate_phase_lut.py --output src/phase_lut.c
```

This creates pre-computed sine/cosine values to speed up steering vector calculations.

### 4. Build and Flash

```bash
mkdir build && cd build
cmake ..
make
st-flash write stm32-beamformer.bin 0x8000000
```

Or use STM32CubeIDE directly (simpler).

### 5. Test

Connect USB serial adapter and monitor output:

```bash
python3 tools/parse_uart_logs.py /dev/ttyUSB0 --stats
```

## Hardware Setup

### ADC Configuration

- **Channels**: PA0-PA7 (4 elements × I/Q pairs)
- **Sample Rate**: 1 MS/s per channel
- **Resolution**: 12-bit
- **Mode**: Scan mode (sequentially samples all channels)
- **DMA**: Circular mode to buffer in RAM

### UART Configuration

- **Port**: UART2 on PB10 (TX) / PB11 (RX)
- **Baud Rate**: 115200
- **Purpose**: Debug output and telemetry

### Signal Chain

```
RF Input → RF Frontend (mixer, ADC bias) → ADC → DMA Buffer → Beamformer → Output → UART
```

## Main Loop

The application follows this cycle:

```c
int main(void) {
    system_init();
    beamformer_init(&config);
    
    while (1) {
        if (adc_frame_ready()) {
            const int16_t *samples = adc_get_buffer();
            beamform_result_t result = beamformer_process_frame(samples, SAMPLES_PER_FRAME);
            uart_printf("Frame: %lu, Power: %lu dB, Quality: %lu%%\n",
                        result.frame_count, result.power_db, result.quality_percent);
            adc_clear_ready();
        }
    }
}
```

## Configuration Parameters

Key settings in `config-stm32-beamformer.h`:

| Parameter | Default | Purpose |
|-----------|---------|---------|
| CENTER_FREQUENCY_HZ | 2.8e9 | RF frequency (affects wavelength) |
| ARRAY_SPACING_M | 0.0535 | Distance between elements (0.5λ optimal) |
| BEAMFORMING_MODE | 0 | 0=Conventional, 1=MVDR |
| SAMPLES_PER_FRAME | 256 | Samples to process per cycle |
| MVDR_NUM_SNAPSHOTS | 256 | Covariance matrix snapshots |
| MVDR_DIAGONAL_LOAD | 0.01 | Regularization factor |
| DEBUG_LEVEL | 2 | 0=Silent, 3=Verbose |

## Validation

Use Python simulation to validate before hardware:

```bash
python3 tools/validate_beamformer.py --mode conventional --angle 30 --elements 4
```

This simulates beamforming and verifies expected output.

## Performance Targets

- **Beamform Rate**: 2.5 kHz (500 Hz per beamform)
- **Latency**: <300 µs end-to-end
- **RAM**: ~40 KB / 192 KB available
- **Flash**: ~50 KB / 256 KB available

## Next Steps

1. Read `API.md` for function reference
2. Review `TROUBLESHOOTING.md` if issues arise
3. Check `BUILD.md` for advanced build options
4. Explore beamforming modes in `src/beamformer.c`
