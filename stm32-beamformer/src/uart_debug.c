/**
 * @file uart_debug.c
 * @brief UART implementation for STM32F4
 *
 * Uses UART2 on PB10/PB11 (or configured by STM32CubeIDE)
 */

#include "uart_debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* UART handle (initialized by STM32CubeIDE) */
/* UART_HandleTypeDef huart2; */

static char printf_buffer[256];

void uart_init(uint32_t baudrate) {
    /* Stub: actual init handled by STM32CubeIDE */
    /* In STM32CubeIDE:
     * - Configure UART2 on PB10 (TX) / PB11 (RX)
     * - Set baudrate (typically 115200)
     * - Enable interrupt if needed
     * - Generated code: huart2 handle ready
     */
}

int uart_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(printf_buffer, sizeof(printf_buffer) - 1, fmt, args);
    va_end(args);

    if (len > 0) {
        uart_send(printf_buffer);
    }
    return len;
}

void uart_send(const char *str) {
    if (!str) return;

    for (int i = 0; str[i]; i++) {
        uart_putc(str[i]);
    }
}

void uart_putc(char c) {
    /* Stub: actual transmission via HAL
     * HAL_UART_Transmit(&huart2, (uint8_t*)&c, 1, 1000);
     */
}

int uart_gets(char *buffer, int max_len) {
    /* Stub: blocking read with timeout
     * HAL_UART_Receive(&huart2, (uint8_t*)buffer, max_len, 10000);
     */
    return 0;
}

/* Redirect printf to UART */
int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        uart_putc(ptr[i]);
    }
    return len;
}
