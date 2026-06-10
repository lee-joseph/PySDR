/**
 * @file uart_debug.h
 * @brief UART logging and debug output
 */

#ifndef UART_DEBUG_H
#define UART_DEBUG_H

#include <stdint.h>

/* Initialize UART for debug output */
void uart_init(uint32_t baudrate);

/* Send formatted output (like printf) */
int uart_printf(const char *fmt, ...);

/* Send raw string */
void uart_send(const char *str);

/* Send single character */
void uart_putc(char c);

/* Read line from UART (blocking) */
int uart_gets(char *buffer, int max_len);

#endif // UART_DEBUG_H
