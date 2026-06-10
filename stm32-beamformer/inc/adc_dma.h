/**
 * @file adc_dma.h
 * @brief ADC and DMA configuration for I/Q sampling
 */

#ifndef ADC_DMA_H
#define ADC_DMA_H

#include <stdint.h>

#define NUM_CHANNELS 8      /* 4 elements × 2 (I/Q) */
#define SAMPLES_PER_FRAME 256
#define ADC_BUFFER_SIZE (NUM_CHANNELS * SAMPLES_PER_FRAME)

typedef struct {
    uint16_t buffer[ADC_BUFFER_SIZE];
    volatile uint32_t frame_count;
    volatile int ready;
} adc_state_t;

/* Initialize ADC and DMA */
int adc_init(void);

/* Check if frame is ready */
int adc_frame_ready(void);

/* Get current ADC buffer */
const uint16_t* adc_get_buffer(void);

/* Clear ready flag */
void adc_clear_ready(void);

/* ADC interrupt handler (called from ISR) */
void adc_dma_irq_handler(void);

extern adc_state_t g_adc_state;

#endif // ADC_DMA_H
