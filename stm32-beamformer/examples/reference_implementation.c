/**
 * STM32F446RE Phased Array Beamformer - Complete Working Example
 *
 * This example demonstrates a 4-element linear array beamformer
 * for microwave radar applications (IF processing).
 *
 * Hardware: STM32F446RE Nucleo board
 * Compiler: ARM GCC (STM32CubeIDE recommended)
 */

#include "stm32f4xx.h"
#include "beamformer.h"
#include <math.h>
#include <stdio.h>

// ===== Global State =====

// ADC DMA circular buffer: two processing halves, each SAMPLES_PER_FRAME long.
// Each frame contains 4 complex elements: I0,Q0,I1,Q1,I2,Q2,I3,Q3.
volatile int16_t adc_buffer[ADC_BUFFER_SAMPLES];

// Beamformer state
beamformer_state_t beamformer_state = {0};

// DMA completion flags
volatile uint32_t dma_half_complete = 0;
volatile uint32_t dma_full_complete = 0;

// Output results buffer
volatile beamform_result_t last_result = {0};

static void beamformer_update_weights(void);

// ===== System Clock Configuration =====

void SystemClockConfig(void) {
    // Configure system clock to 180 MHz
    // PLL: 16 MHz (HSI) → 180 MHz

    RCC->CR |= RCC_CR_HSION;  // Enable HSI
    while (!(RCC->CR & RCC_CR_HSIRDY));

    // Configure PLL
    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (16 << 0);     // PLLM = 16 (input divider)
    RCC->PLLCFGR |= (360 << 6);    // PLLN = 360 (multiplier)
    RCC->PLLCFGR |= (0 << 16);     // PLLP = 2 (output divider)
    RCC->PLLCFGR |= (7 << 24);     // PLLQ = 8

    RCC->CR |= RCC_CR_PLLON;  // Enable PLL
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Configure flash wait states and prefetch
    FLASH->ACR |= FLASH_ACR_LATENCY_5WS;
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    // APB1 = 45 MHz, APB2 = 90 MHz once SYSCLK is 180 MHz
    RCC->CFGR &= ~((7 << 10) | (7 << 13));
    RCC->CFGR |= (5 << 10);  // APB1 divider /4
    RCC->CFGR |= (4 << 13);  // APB2 divider /2

    // Select PLL as system clock
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= (2 << 0);  // PLL selected
    while ((RCC->CFGR & RCC_CFGR_SWS) != (2 << 2));
}

// ===== GPIO Configuration =====

void GPIO_Init(void) {
    // Enable GPIOA, GPIOB, GPIOC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    // PA0-PA7: ADC inputs (analog)
    GPIOA->MODER |= (3 << 0);   // PA0 analog
    GPIOA->MODER |= (3 << 2);   // PA1 analog
    GPIOA->MODER |= (3 << 4);   // PA2 analog
    GPIOA->MODER |= (3 << 6);   // PA3 analog
    GPIOA->MODER |= (3 << 8);   // PA4 analog
    GPIOA->MODER |= (3 << 10);  // PA5 analog
    GPIOA->MODER |= (3 << 12);  // PA6 analog
    GPIOA->MODER |= (3 << 14);  // PA7 analog

    // PB10: USART3 TX (for debugging)
    GPIOB->MODER |= (2 << 20);  // PB10 alternate function
    GPIOB->AFR[1] |= (7 << 8);  // AF7 = USART3
}

// ===== ADC Configuration =====

void ADC_Init(void) {
    // Enable ADC1. The STM32F446RE has three ADC peripherals, but this example
    // uses one ADC scanning all eight I/Q inputs into a deterministic buffer.
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // ADC clock = APB2 / 4 = 22.5 MHz
    ADC->CCR |= (1 << 16);  // Prescaler /4

    // === ADC1 Configuration ===
    ADC1->CR1 = 0;
    ADC1->CR1 |= ADC_CR1_SCAN;      // Scan mode
    ADC1->CR2 = 0;
    ADC1->CR2 |= ADC_CR2_CONT;      // Continuous conversion
    ADC1->CR2 |= ADC_CR2_DMA;       // DMA enabled
    ADC1->CR2 |= ADC_CR2_DMACM;     // DMA in circular mode

    // Sequence: IN0-IN7 on PA0-PA7:
    // I0, Q0, I1, Q1, I2, Q2, I3, Q3.
    ADC1->SQR1 = (7 << 20);  // 8 conversions
    ADC1->SQR3 |= (0 << 0);  // Rank 1: IN0
    ADC1->SQR3 |= (1 << 5);  // Rank 2: IN1
    ADC1->SQR3 |= (2 << 10); // Rank 3: IN2
    ADC1->SQR3 |= (3 << 15); // Rank 4: IN3
    ADC1->SQR3 |= (4 << 20); // Rank 5: IN4
    ADC1->SQR3 |= (5 << 25); // Rank 6: IN5
    ADC1->SQR2 |= (6 << 0);  // Rank 7: IN6
    ADC1->SQR2 |= (7 << 5);  // Rank 8: IN7

    // Sample time = 28.5 cycles (high accuracy)
    ADC1->SMPR2 |= (3 << 0);   // IN0
    ADC1->SMPR2 |= (3 << 3);   // IN1
    ADC1->SMPR2 |= (3 << 6);   // IN2
    ADC1->SMPR2 |= (3 << 9);   // IN3
    ADC1->SMPR2 |= (3 << 12);  // IN4
    ADC1->SMPR2 |= (3 << 15);  // IN5
    ADC1->SMPR2 |= (3 << 18);  // IN6
    ADC1->SMPR2 |= (3 << 21);  // IN7

    // Power on
    ADC1->CR2 |= ADC_CR2_ADON;

    // Wait for stabilization
    for (uint32_t i = 0; i < 1000000; i++);
}

// ===== DMA Configuration =====

void DMA_Init(void) {
    // Enable DMA2
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    // Disable stream first
    DMA2_Stream0->CR &= ~DMA_SxCR_EN;
    while (DMA2_Stream0->CR & DMA_SxCR_EN);

    // Clear flags
    DMA2->LIFCR = 0xFFFFFFFF;

    // Configuration
    DMA2_Stream0->CR = 0;
    DMA2_Stream0->CR |= (0 << 25);      // Channel 0 (ADC1)
    DMA2_Stream0->CR |= (1 << 13);      // Memory data size: 16-bit halfword
    DMA2_Stream0->CR |= (1 << 11);      // Peripheral data size: 16-bit
    DMA2_Stream0->CR |= (1 << 9);       // Memory increment
    DMA2_Stream0->CR |= (1 << 8);       // Circular mode
    DMA2_Stream0->CR |= (3 << 16);      // Very high priority

    // Number of transfers
    DMA2_Stream0->NDTR = ADC_BUFFER_SAMPLES;

    // Addresses
    DMA2_Stream0->PAR = (uint32_t)&ADC1->DR;  // Peripheral (ADC data)
    DMA2_Stream0->M0AR = (uint32_t)adc_buffer;  // Memory

    // Interrupts: half-transfer and transfer complete
    DMA2_Stream0->CR |= (1 << 5);  // HTIE (half transfer)
    DMA2_Stream0->CR |= (1 << 4);  // TCIE (transfer complete)

    // Enable stream
    DMA2_Stream0->CR |= DMA_SxCR_EN;

    // Enable interrupt in NVIC
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    NVIC_SetPriority(DMA2_Stream0_IRQn, 2);
}

// ===== UART Configuration (for debugging) =====

void UART_Init(void) {
    // Enable USART3 clock
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

    // USART3 configuration
    USART3->CR1 = 0;
    USART3->CR1 |= USART_CR1_TE;      // Transmitter enabled
    USART3->CR1 |= USART_CR1_UE;      // UART enabled

    // Baud rate = 115200
    // Baud = fclk / (16 * USARTDIV)
    // USARTDIV = 45,000,000 / (16 * 115200) ≈ 24.4
    USART3->BRR = (24 << 4) | 7;  // Mantissa=24, Fraction=7
}

void UART_SendByte(uint8_t byte) {
    while (!(USART3->SR & USART_SR_TXE));
    USART3->DR = byte;
}

void UART_SendString(const char *str) {
    while (*str) {
        UART_SendByte(*str++);
    }
}

// ===== Interrupt Handlers =====

void DMA2_Stream0_IRQHandler(void) {
    if (DMA2->LISR & DMA_LISR_HTIF0) {
        // Half transfer complete - first half of buffer ready
        DMA2->LIFCR = DMA_LIFCR_CHTIF0;
        dma_half_complete = 1;
    }
    if (DMA2->LISR & DMA_LISR_TCIF0) {
        // Transfer complete - second half of buffer ready
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;
        dma_full_complete = 1;
    }
}

// ===== Beamformer Functions =====

void beamformer_init(float theta_rad, float d_over_lambda) {
    beamformer_state.target_theta = theta_rad;
    beamformer_state.d_over_wavelength = d_over_lambda;
    beamformer_state.frame_count = 0;
    phase_lut_init();
    beamformer_update_weights();
}

static void beamformer_update_weights(void) {
    for (int n = 0; n < NUM_ELEMENTS; n++) {
        // steering_phase = 2π * d/λ * sin(θ) * n
        float phase_rad = 2.0f * 3.14159f *
                         beamformer_state.d_over_wavelength *
                         sinf(beamformer_state.target_theta) * n;

        // Normalize to [0, 2π)
        while (phase_rad >= 2.0f * 3.14159f) phase_rad -= 2.0f * 3.14159f;
        while (phase_rad < 0) phase_rad += 2.0f * 3.14159f;

        // Convert to lookup table index
        uint16_t phase_idx = (uint16_t)((phase_rad / (2.0f * 3.14159f)) * PHASE_MAX);

        // Get cos/sin from LUT (would be implemented in phase_calc.c)
        // For now, inline calculation
        float cos_val = cosf(phase_rad);
        float sin_val = sinf(phase_rad);

        beamformer_state.weights[n].real = (int32_t)(cos_val * (1 << FP_BITS));
        beamformer_state.weights[n].imag = (int32_t)(sin_val * (1 << FP_BITS));
    }
}

complex_fp_t fp_cmul_conj(complex_fp_t a, complex_fp_t b) {
    complex_fp_t result;
    // (a_r + j*a_i) * (b_r - j*b_i) = (a_r*b_r + a_i*b_i) + j(a_i*b_r - a_r*b_i)
    result.real = ((a.real * b.real) + (a.imag * b.imag)) >> FP_BITS;
    result.imag = ((a.imag * b.real) - (a.real * b.imag)) >> FP_BITS;
    return result;
}

uint32_t complex_power(complex_fp_t c) {
    int32_t re = c.real >> 10;
    int32_t im = c.imag >> 10;
    return (uint32_t)((re * re) + (im * im));
}

beamform_result_t beamformer_process_frame(const int16_t *frame_buffer) {
    beamform_result_t result = {0, 0, 0};
    uint32_t power_accum = 0;

    // Process each I/Q sample pair
    // Buffer layout: [I0, Q0, I1, Q1, I2, Q2, I3, Q3, I0, Q0, ...]
    for (int i = 0; i < SAMPLES_PER_FRAME; i++) {
        complex_fp_t beam_out = {0, 0};

        for (int ch = 0; ch < NUM_ELEMENTS; ch++) {
            // Extract I/Q from buffer
            int16_t i_samp = frame_buffer[i * ADC_CHANNELS_PER_FRAME + ch * 2];
            int16_t q_samp = frame_buffer[i * ADC_CHANNELS_PER_FRAME + ch * 2 + 1];

            // Convert to fixed-point complex
            complex_fp_t received;
            received.real = ((int32_t)i_samp) << 6;
            received.imag = ((int32_t)q_samp) << 6;

            // Apply weight and accumulate
            complex_fp_t weighted = fp_cmul_conj(beamformer_state.weights[ch], received);
            beam_out.real += weighted.real;
            beam_out.imag += weighted.imag;
        }

        // Calculate power
        uint32_t power = complex_power(beam_out);
        power_accum += power;
    }

    // Average power
    result.power = power_accum / SAMPLES_PER_FRAME;

    // Convert to dB (10 * log10(power))
    if (result.power > 0) {
        // Approximate: log10(x) ≈ log2(x) * 0.301
        uint32_t msb = 31 - __builtin_clz(result.power);
        result.power_db = (int16_t)(30 + msb * 3);
    }

    result.quality_percent = 95;  // Placeholder
    beamformer_state.frame_count++;

    return result;
}

void phase_lut_init(void) {
    // Placeholder: LUT initialization would go here
    // In production, create at compile-time or during startup
}

void beamformer_steer(float new_theta_rad) {
    beamformer_state.target_theta = new_theta_rad;
    beamformer_update_weights();
}

// ===== Main Application =====

int main(void) {
    // System initialization
    SystemClockConfig();
    GPIO_Init();
    UART_Init();
    ADC_Init();
    DMA_Init();

    // Beamformer initialization
    // For 2.8 GHz: λ = 10.7 cm, d = 5.35 cm = 0.5λ
    float d_over_wavelength = 0.5f;
    float steering_angle = 0.0f;  // Boresight (0 degrees)

    beamformer_init(steering_angle, d_over_wavelength);

    UART_SendString("\r\n=== STM32F446RE Beamformer Starting ===\r\n");

    // Start ADC conversion
    ADC1->CR2 |= ADC_CR2_SWSTART;

    // Main loop
    uint32_t loop_count = 0;
    while (1) {
        // Process half buffer
        if (dma_half_complete) {
            dma_half_complete = 0;
            last_result = beamformer_process_frame((const int16_t*)adc_buffer);
        }

        // Process full buffer
        if (dma_full_complete) {
            dma_full_complete = 0;
            last_result = beamformer_process_frame(
                (const int16_t*)&adc_buffer[ADC_CHANNELS_PER_FRAME * SAMPLES_PER_FRAME]
            );
        }

        // Debug output every 100 frames
        loop_count++;
        if (loop_count >= 100) {
            loop_count = 0;

            char buf[100];
            sprintf(buf, "Frame: %lu, Power: %d dB, Quality: %u%%\r\n",
                   beamformer_state.frame_count,
                   last_result.power_db,
                   last_result.quality_percent);
            UART_SendString(buf);

            // Steer beam every 10 outputs
            if ((beamformer_state.frame_count / 100) % 10 == 0) {
                steering_angle += 0.1f;  // +5.7 degrees per cycle
                if (steering_angle > 3.14159f) steering_angle = -3.14159f;
                beamformer_steer(steering_angle);
            }
        }
    }

    return 0;
}
