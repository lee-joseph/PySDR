# Troubleshooting Guide

## Issue: "ADC data looks like noise"

**Symptoms:** Raw ADC values don't show clear signal pattern, appear random

**Solutions:**
1. **Check RF signal presence:**
   ```bash
   # Use oscilloscope to measure at RF frontend output
   # Should see signal at specified IF frequency
   ```

2. **Verify bias network:**
   - Measure voltage at ADC pin (PA0-PA7) with multimeter
   - Should be ~1.65V DC (mid-rail)
   - If not, check 1.65V bias voltage source

3. **Check AC coupling:**
   - Verify 1µF coupling capacitor is connected
   - Test with capacitor directly across signal and input

4. **Signal level adjustment:**
   - Peak signal should be ±500 mV
   - Too small: increase LNA gain
   - Too large: check bias network offset

---

## Issue: "Beamformer output is constant regardless of steering angle"

**Symptoms:** Power output doesn't change when calling `beamformer_steer()`

**Solutions:**
1. **Enable debug output:**
   ```c
   // In config.h
   #define DEBUG_LEVEL 3
   ```

2. **Check steering vector calculation:**
   - Verify array spacing in config.h matches actual hardware
   - For 2.8 GHz: spacing should be ~0.0535 m (0.5λ)

3. **Test phase calculation manually:**
   ```c
   complex_fp_t steer_vec[4];
   steering_vector_calc(0.5f, 2.8e9, 0.0535, 4, steer_vec);
   // Print values to verify they change with angle
   ```

4. **Validate with known source:**
   - Place RF source at specific angle (use RF goniometer)
   - Measure power vs. steering angle
   - Should see peak power at correct angle

---

## Issue: "UART output is garbled"

**Symptoms:** Serial output shows random characters or is unreadable

**Solutions:**
1. **Check baud rate:**
   - Verify setting: 115200 baud (8-N-1)
   - In config.h: `#define UART_BAUD_RATE 115200`

2. **Check physical connections:**
   - Verify USB-to-serial adapter is 3.3V (not 5V)
   - TX from STM32 → RX on adapter
   - GND → GND
   - No crossover needed

3. **Try different USB cable:**
   - Some cables have poor shielding
   - Replace with shielded cable

4. **Check clock configuration:**
   - System clock must be 180 MHz
   - UART baud calculation depends on this

---

## Issue: "Board won't flash / ST-Link not detected"

**Symptoms:** `st-flash: command not found` or "STM32 not found"

**Solutions:**
1. **Install st-flash:**
   ```bash
   # macOS
   brew install stlink
   
   # Linux
   sudo apt install stlink-tools
   ```

2. **Check ST-Link connection:**
   - USB cable connected to Nucleo board
   - Should see /dev/ttyUSB* on Linux

3. **Check board jumpers:**
   - Verify CN2 jumpers are in correct position
   - Default position enables ST-Link

4. **Try alternative flasher:**
   ```bash
   # Using OpenOCD
   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
   ```

---

## Issue: "Covariance matrix ill-conditioned" (MVDR mode)

**Symptoms:** MVDR weights become unstable or produce zeros

**Solutions:**
1. **Increase diagonal loading:**
   ```c
   // In config.h
   #define MVDR_DIAGONAL_LOAD 0.05f  // Increase from 0.01
   ```

2. **Collect more snapshots:**
   ```c
   #define MVDR_NUM_SNAPSHOTS 512  // Increase from 256
   ```

3. **Check for colored noise:**
   - MVDR assumes white noise
   - If interference is correlated, may need preprocessing

---

## Issue: "Beam pattern has high sidelobes"

**Symptoms:** Beam has unwanted peaks in other directions

**Solutions:**
1. **Apply tapering:**
   ```c
   // In config.h
   #define TAPER_TYPE 1  // 1=Hanning, 2=Hamming, 3=Blackman
   ```

2. **Increase array size:**
   - More elements → narrower mainlobe and lower sidelobes
   - Limited by hardware

3. **Check element spacing:**
   - Verify half-wavelength spacing (0.0535 m @ 2.8 GHz)
   - Spacing errors cause grating lobes

---

## Issue: "Processing latency is too high"

**Symptoms:** Time from ADC sample to output exceeds requirements

**Solutions:**
1. **Profile the code:**
   ```c
   #define ENABLE_PROFILING 1  // In config.h
   ```

2. **Reduce sample buffer size:**
   ```c
   #define SAMPLES_PER_FRAME 128  // Reduce from 256
   ```

3. **Optimize beamformer:**
   - Use conventional mode instead of MVDR
   - Disable debug output in production

4. **Check CPU load:**
   - Monitor with profiler
   - May need faster MCU or FPGA co-processor

---

## Debug Techniques

### UART Logging
Enable detailed logging:
```c
#define DEBUG_LEVEL 3
#define LOG_RAW_ADC_SAMPLES 1
#define ENABLE_PROFILING 1
```

### GDB Debugging
```bash
# Terminal 1: Start OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# Terminal 2: GDB
arm-none-eabi-gdb build/stm32-beamformer.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) break beamformer_process_frame
(gdb) c
(gdb) p beamformer_state
```

### Oscilloscope Measurements
- Measure IF signal at RF frontend output
- Monitor ADC input bias voltage (1.65V)
- Check UART TX signal (should be RS-232 levels inverted via adapter)

---

## Getting Help

1. Check this guide first
2. Enable DEBUG_LEVEL=3 and review output
3. Open GitHub issue with:
   - Error message or symptoms
   - Config settings (frequency, spacing, mode)
   - UART debug output
4. Contact maintainers

---

## Common Parameter Issues

| Parameter | Too Low | Too High |
|-----------|---------|----------|
| ADC Sample Rate | Aliasing | High power consumption |
| MVDR Diagonal Load | Ill-conditioning | Loss of adaptivity |
| Beam Scan Step | Slow scanning | Missed detections |
| Debug Level | Missing info | Excessive UART traffic |

