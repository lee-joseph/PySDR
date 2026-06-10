# STM32 Beamformer: Setup & Build Guide

## Prerequisites

### 1. ARM Toolchain

Install the ARM Cortex-M4 compiler:

**macOS:**
```bash
brew install arm-none-eabi-gcc arm-none-eabi-gdb
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install gcc-arm-none-eabi gdb-arm-none-eabi binutils-arm-none-eabi
```

**Windows:**
Download from [ARM Developer](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)

Verify installation:
```bash
arm-none-eabi-gcc --version
# Should show: arm-none-eabi-gcc (GNU Arm Embedded Toolchain 10.3-2021.10) ...
```

### 2. Build Tools

**CMake (3.13+):**
```bash
brew install cmake          # macOS
sudo apt install cmake      # Linux
```

**Make:**
Usually pre-installed. Check:
```bash
make --version
```

**ST-Link Utility (for flashing):**
```bash
brew install stlink         # macOS
sudo apt install stlink-tools  # Linux
```

Or download from [GitHub](https://github.com/stlink-org/stlink/releases)

### 3. Optional: STM32CubeMX

For visual hardware configuration: [Download STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)

---

## Project Layout

```
stm32-beamformer/
├── src/                     # Source code
├── inc/                     # Headers
├── lib/                     # Libraries (STM32 startup, CMSIS)
├── CMakeLists.txt          # Build configuration
├── config.h                 # User-configurable parameters
└── docs/
    └── SETUP.md            # This file
```

---

## Building the Firmware

### Step 1: Clone Repository

```bash
git clone <repo-url> stm32-beamformer
cd stm32-beamformer
```

### Step 2: Create Build Directory

```bash
mkdir build && cd build
```

### Step 3: Configure with CMake

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/stm32-toolchain.cmake
```

Expected output:
```
STM32 Beamformer Build Configuration
=====================================
MCU: STM32F446RE
Compiler: arm-none-eabi-gcc
Optimization: -O2
...
```

### Step 4: Compile

```bash
make -j4
```

Output files created:
- `stm32-beamformer.elf` — Executable (with debug symbols)
- `stm32-beamformer.bin` — Binary for flashing
- `stm32-beamformer.hex` — HEX format for programming

Check size:
```bash
arm-none-eabi-size stm32-beamformer.elf
# Text  | Data  | BSS   | Total
# 28544 | 1024  | 8192  | 37760 bytes
```

---

## Flashing to Board

### Option 1: Using st-flash (Recommended)

```bash
st-flash write stm32-beamformer.bin 0x8000000
```

Output:
```
st-flash 1.7.0
2024-01-15T10:30:42 INFO common.c: STM32F446RE_FLASH_SIZE = 256 KB
2024-01-15T10:30:42 INFO flash_loader.c: Using flash loader in SRAM
2024-01-15T10:30:42 INFO flash_loader.c: Using 64-byte write block
2024-01-15T10:30:42 INFO flash_loader.c: Finished erasing 2 blocks
2024-01-15T10:30:42 INFO common.c: Wrote 28544 bytes at address 0x8000000 in 0.689 seconds
```

### Option 2: Using STM32CubeProgrammer

```bash
STM32_Programmer_CLI -c port=SWD -d stm32-beamformer.bin -v
```

### Option 3: Using OpenOCD

```bash
# Start OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# In another terminal, run GDB
arm-none-eabi-gdb stm32-beamformer.elf

# In GDB:
(gdb) target remote localhost:3333
(gdb) load
(gdb) c
```

---

## Verifying the Build

### Connect Serial Monitor

Connect USB cable to Nucleo board and open serial terminal:

```bash
# macOS/Linux
screen /dev/ttyUSB0 115200
# or
picocom -b 115200 /dev/ttyUSB0

# Windows (use PuTTY or TeraTerm)
```

### Expected Output

```
=== STM32F446RE Beamformer Starting ===
Frame: 100, Power: 45 dB, Quality: 95%
Frame: 200, Power: 46 dB, Quality: 95%
Frame: 300, Power: 44 dB, Quality: 94%
...
```

If you see ADC/phase errors or "No Signal", check:
- RF frontend power supply
- Bias network voltage (should be ~1.65V)
- Signal cable connections
- ADC sample rate configuration

---

## Hardware Setup

### RF Frontend Connections

Connect RF down-converter outputs to STM32 ADC inputs:

| Channel | Signal | STM32 Pin | Notes |
|---------|--------|-----------|-------|
| 0 | I (In-phase) | PA0 | I/Q pair for element 0 |
| 0 | Q (Quadrature) | PA1 | |
| 1 | I | PA2 | I/Q pair for element 1 |
| 1 | Q | PA3 | |
| 2 | I | PA4 | I/Q pair for element 2 |
| 2 | Q | PA5 | |
| 3 | I | PA6 | I/Q pair for element 3 |
| 3 | Q | PA7 | |
| Debug | TX | PB10 | Serial output (115200 baud) |

### Bias Network (Critical!)

ADC input must be **AC-coupled and biased**:

```
IF Input ──┬──┬── 1µF ──┬── [Bias Network] ──┬── PA0
           │  │         │                    │
           │ [1µ to GND] [100k to 1.65V]   [10k pull-down]
```

Bias voltage should be **1.65V** (midpoint of 0-3.3V ADC range).

### LDO Power Supply

Use **3.3V LDO** for ADC bias network:
- LT1117 or AMS1117 recommended
- Capacitors: 10µF input, 10µF output

---

## Configuration

Edit `inc/config.h` to customize:

```c
// Set beamforming mode
#define BEAMFORMING_MODE 0  // 0=Conventional, 1=MVDR

// Set center frequency
#define CENTER_FREQUENCY_HZ 2.8e9  // 2.8 GHz X-band

// Set array spacing
#define ARRAY_SPACING_M 0.0535  // 0.5 wavelength

// Enable debug logging
#define DEBUG_LEVEL 2  // 0=off, 1=errors, 2=info, 3=verbose
```

Then rebuild:
```bash
cd build
make clean
make -j4
```

---

## Debugging

### Enable Debug Output

In `config.h`:
```c
#define DEBUG_LEVEL 3  // Verbose output
#define LOG_RAW_ADC_SAMPLES 1  // Log raw I/Q data
#define ENABLE_PROFILING 1     // Measure CPU time
```

### View Live Data

Use Python script to parse and plot UART output:

```bash
python3 tools/parse_uart_logs.py /dev/ttyUSB0 --plot
```

This creates a live plot of:
- Beamformer output power vs. time
- Steering angle
- Processing latency

### GDB Debugging

Start OpenOCD and GDB:

```bash
# Terminal 1: Start OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# Terminal 2: GDB
arm-none-eabi-gdb build/stm32-beamformer.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) break main
(gdb) c
(gdb) p beamformer_state  # Print beamformer state
(gdb) p/x $sp             # Print stack pointer
```

---

## Troubleshooting

### Problem: "st-flash: command not found"

**Solution:** Install st-flash
```bash
brew install stlink          # macOS
sudo apt install stlink-tools  # Linux
```

### Problem: "arm-none-eabi-gcc: command not found"

**Solution:** Install ARM toolchain (see Prerequisites)

### Problem: "CMake: command not found"

**Solution:** 
```bash
brew install cmake       # macOS
sudo apt install cmake   # Linux
```

### Problem: Build fails with "undefined reference"

**Solution:** Check linker script path in CMakeLists.txt matches actual location

### Problem: ADC data looks like noise

**Solution:** 
1. Check RF signal level (should be ~±500 mV at ADC input)
2. Verify bias network is centered at 1.65V
3. Check AC coupling capacitor value (1µF)
4. Try with test signal before using RF frontend

### Problem: Beamformer output doesn't change with steering angle

**Solution:**
1. Verify array spacing in config.h
2. Check phase calculation: enable DEBUG_LEVEL=3
3. Validate steering_vector.c generates correct phase shifts
4. Measure phase with: `beam_pattern_at_angle(angle_rad)`

### Problem: UART output is garbled

**Solution:**
- Check baud rate: must be 115200
- Verify USB-to-serial adapter voltage (3.3V)
- Check RX/TX connections (no crossover needed)
- Try different USB cable

---

## Performance Metrics

Expected performance on STM32F446RE:

| Metric | Value |
|--------|-------|
| Frame rate | 3.9 kHz (without overhead) |
| Effective beamform rate | ~2.5 kHz |
| Processing latency | 150 µs |
| ADC sampling rate | 1 MHz |
| Output power precision | ±0.5 dB |
| Beam steering speed | Real-time (<1 ms) |
| CPU load | ~60% (at 2.5 kHz) |
| RAM usage | ~30 KB / 192 KB |
| Flash usage | ~50 KB / 256 KB |

---

## Next Steps

1. **Test with known signal** — Use function generator + RF mixer
2. **Validate beamformer** — Measure gain vs. steering angle
3. **Calibrate array** — Run `calibration_at_boresight()`
4. **Deploy** — Integrate into your radar/comms system

---

## References

- STM32F446RE Datasheet: https://www.st.com/datasheet/stm32f446
- ARM Cortex-M4 Generic User Guide: https://developer.arm.com/documentation
- PySDR Beamforming Chapter: [PySDR Textbook]
- CMake Build Documentation: https://cmake.org/cmake/help/latest/

---

## Support

For issues or questions:
1. Check `docs/TROUBLESHOOTING.md`
2. Enable `DEBUG_LEVEL=3` and review logs
3. Open GitHub issue with error output
4. Contact maintainers (see README)
