cmake_minimum_required(VERSION 3.13)
project(stm32-beamformer C ASM)

# ============================================================================
# Project Configuration
# ============================================================================

set(PROJECT_VERSION "1.0.0")
set(MCU "STM32F446RE")
set(MCU_CORE "cortex-m4")

# Compiler and flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ============================================================================
# Toolchain Settings (ARM GCC)
# ============================================================================

set(ARM_TOOLCHAIN_PREFIX "arm-none-eabi-")
set(CMAKE_C_COMPILER ${ARM_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${ARM_TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${ARM_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${ARM_TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE ${ARM_TOOLCHAIN_PREFIX}size)

# ============================================================================
# Optimization and Warning Flags
# ============================================================================

set(OPTIMIZATION_FLAGS "-O2")  # Balance speed and size
set(WARNING_FLAGS "-Wall -Wextra -Wpedantic -Wconversion")
set(DEBUG_FLAGS "-g3 -gdwarf-2")

# Common flags for all configurations
set(CMAKE_C_FLAGS "${OPTIMIZATION_FLAGS} ${WARNING_FLAGS} ${DEBUG_FLAGS}")

# ARM-specific flags
set(ARM_FLAGS
    "-mcpu=${MCU_CORE}"
    "-mthumb"
    "-mfpu=fpv4-sp-d16"
    "-mfloat-abi=hard"
    "-fno-exceptions"
    "-fno-unwind-tables"
    "-ffunction-sections"
    "-fdata-sections"
)

foreach(FLAG ${ARM_FLAGS})
    string(APPEND CMAKE_C_FLAGS " ${FLAG}")
endforeach()

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS
    "-Wl,--gc-sections"
    "-Wl,--print-memory-usage"
    "-nostartfiles"
)

# ============================================================================
# Source Files
# ============================================================================

# Core beamformer sources
set(BEAMFORMER_SOURCES
    src/main.c
    src/system_init.c
    src/adc_dma.c
    src/beamformer.c
    src/steering_vector.c
    src/fixed_point_math.c
    src/calibration.c
    src/uart_debug.c
    lib/stm32_startup.c
)

# Include directories
set(BEAMFORMER_INCLUDES
    inc/
    lib/CMSIS/
)

# ============================================================================
# Executable Target
# ============================================================================

add_executable(${PROJECT_NAME}.elf ${BEAMFORMER_SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${BEAMFORMER_INCLUDES})

# Linker script
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/lib/stm32f446re_linker.ld")
target_link_options(${PROJECT_NAME}.elf PRIVATE
    -T${LINKER_SCRIPT}
)

# ============================================================================
# Post-Build Commands
# ============================================================================

# Generate binary (HEX and BIN formats)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf>
            ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf>
            ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
)

# ============================================================================
# Build Targets
# ============================================================================

# Flash target (requires st-flash)
add_custom_target(flash
    COMMAND st-flash write ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin 0x8000000
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing to STM32F446RE..."
)

# Generate phase lookup table
add_custom_target(generate_lut
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/generate_phase_lut.py
            --output ${CMAKE_SOURCE_DIR}/src/phase_lut.c
    COMMENT "Generating phase lookup table..."
)

# Run unit tests (host-based)
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_steering_vector tests/test_steering_vector.c
                   src/steering_vector.c src/fixed_point_math.c)
    target_include_directories(test_steering_vector PRIVATE inc/ lib/CMSIS/)
    add_test(NAME SteeringVector COMMAND test_steering_vector)
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "STM32 Beamformer Build Configuration")
message(STATUS "=====================================")
message(STATUS "MCU: ${MCU}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Optimization: ${OPTIMIZATION_FLAGS}")
message(STATUS "Linker Script: ${LINKER_SCRIPT}")
message(STATUS "")
message(STATUS "Build targets:")
message(STATUS "  make                - Build firmware")
message(STATUS "  make flash          - Flash to board")
message(STATUS "  make generate_lut   - Pre-compute phase table")
message(STATUS "")
