# Build Instructions

## Prerequisites

- **STM32CubeIDE** (or arm-none-eabi-gcc with STM32 HAL)
- **CMake** 3.15+
- **st-flash** (for programming)
- **Python 3** (for utilities)

## Quick Build

### Option 1: STM32CubeIDE (Recommended)

1. Import project into STM32CubeIDE
2. Right-click project → Build Project
3. Connect ST-Link to Nucleo board
4. Right-click project → Run As → STM32 C/C++ Application

### Option 2: Command Line

```bash
mkdir build && cd build
cmake ..
make
make flash
```

This builds:
- `build/stm32-beamformer.elf` - Executable
- `build/stm32-beamformer.bin` - Binary for flashing
- `build/phase_lut.c` - Pre-computed lookup table

## Generating Phase LUT

The phase lookup table is pre-computed to save runtime:

```bash
python3 tools/generate_phase_lut.py --output src/phase_lut.c --entries 4096
```

## Flashing to Board

### Using st-flash

```bash
st-flash write build/stm32-beamformer.bin 0x8000000
```

### Using STM32CubeIDE

The IDE handles flashing automatically via ST-Link.

## Configuration

Edit `inc/config-stm32-beamformer.h` before building:

```c
#define CENTER_FREQUENCY_HZ 2.8e9
#define ARRAY_SPACING_M 0.0535
#define BEAMFORMING_MODE 0     /* 0=Conventional, 1=MVDR */
#define DEBUG_LEVEL 2
```

## Verifying Installation

1. Connect USB serial adapter to PB10/PB11 (UART)
2. Open terminal at 115200 baud
3. Monitor output with Python utility:

```bash
python3 tools/parse_uart_logs.py /dev/ttyUSB0 --stats --plot
```

## Troubleshooting

### Build Errors

- **CMake not found**: Install CMake or use STM32CubeIDE
- **arm-none-eabi-gcc not found**: Install ARM toolchain
- **Missing includes**: Run CMake with -DSTM32_HAL_PATH

### Flash Errors

- **st-flash: command not found**: `brew install stlink` (macOS) or `apt install stlink-tools`
- **ST-Link not detected**: Check USB cable, verify CN2 jumpers on Nucleo board

See `TROUBLESHOOTING.md` for detailed solutions.
