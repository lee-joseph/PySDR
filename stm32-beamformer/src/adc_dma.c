/**
 * @file adc_dma.c
 * @brief ADC and DMA configuration implementation
 *
 * Requires STM32 HAL: stm32f4xx_hal_adc.h, stm32f4xx_hal_dma.h
 */

#include "adc_dma.h"

adc_state_t g_adc_state = {
    .frame_count = 0,
    .ready = 0
};

/* ADC handle (would be initialized by STM32CubeIDE or user code) */
/* ADC_HandleTypeDef hadc1; */
/* DMA_HandleTypeDef hdma_adc1; */

int adc_init(void) {
    /* This function is a stub. In practice:
     * 1. Configure ADC1 for scan mode with 8 channels (PA0-PA7)
     * 2. Set ADC clock to 36 MHz (180 MHz / 5)
     * 3. Set sample time to 84 cycles for phase coherence
     * 4. Enable DMA2 Stream0 in circular mode
     * 5. Set DMA to 2-byte transfers at ADC->DR
     * 6. Start ADC continuous conversion
     *
     * Typical STM32CubeIDE flow:
     * - Use .ioc file to configure ADC1, DMA2_Stream0
     * - Generated code will create hadc1, hdma_adc1
     * - Call HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adc_state.buffer, ADC_BUFFER_SIZE)
     */
    return 0;
}

int adc_frame_ready(void) {
    return g_adc_state.ready;
}

const uint16_t* adc_get_buffer(void) {
    return g_adc_state.buffer;
}

void adc_clear_ready(void) {
    g_adc_state.ready = 0;
}

void adc_dma_irq_handler(void) {
    /* Called from DMA2_Stream0_IRQHandler (half-transfer or full completion) */
    g_adc_state.frame_count++;
    g_adc_state.ready = 1;
}

/* Typical ISR integration (add to stm32f4xx_it.c):
 *
 * void DMA2_Stream0_IRQHandler(void) {
 *     HAL_DMA_IRQHandler(&hdma_adc1);
 *     adc_dma_irq_handler();
 * }
 *
 * void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
 *     if (hadc->Instance == ADC1) {
 *         adc_dma_irq_handler();
 *     }
 * }
 */
