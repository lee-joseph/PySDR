/**
 * @file system_init.c
 * @brief STM32F446RE system initialization
 *
 * Configures clock, GPIO, and core peripherals.
 */

#include "stm32f4xx.h"
#include "system_init.h"

void system_clock_init(void) {
    // Enable HSI
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    // Configure PLL: 16 MHz HSI → 180 MHz
    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (16 << 0);     // PLLM = 16
    RCC->PLLCFGR |= (360 << 6);    // PLLN = 360
    RCC->PLLCFGR |= (0 << 16);     // PLLP = 2
    RCC->PLLCFGR |= (7 << 24);     // PLLQ = 8

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Configure flash
    FLASH->ACR |= FLASH_ACR_LATENCY_5WS;
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    // APB dividers
    RCC->CFGR &= ~((7 << 10) | (7 << 13));
    RCC->CFGR |= (5 << 10);  // APB1: /4 → 45 MHz
    RCC->CFGR |= (4 << 13);  // APB2: /2 → 90 MHz

    // Select PLL
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= (2 << 0);
    while ((RCC->CFGR & RCC_CFGR_SWS) != (2 << 2));
}

void gpio_init(void) {
    // Enable GPIO clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // PA0-PA7: ADC inputs (analog mode)
    for (int i = 0; i < 8; i++) {
        GPIOA->MODER |= (3 << (i * 2));
    }

    // PB10: USART3 TX
    GPIOB->MODER |= (2 << 20);
    GPIOB->AFR[1] |= (7 << 8);
}

int system_init(void) {
    system_clock_init();
    gpio_init();
    return 0;
}
