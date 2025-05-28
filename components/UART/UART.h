/**
 * @file UART.h
 * @author your name (you@domain.com)
 * @brief this file contains the header for UART communication using
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef RECIVE_UART_H
#define RECIVE_UART_H

#include "../my_config/my_config.h"

#if TEST_ON_PC == 0
#include "stdio.h"
#include "stdbool.h"
#include "stdarg.h" // Needed for va_list
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp8266/pin_mux_register.h"
#include "esp_task_wdt.h"

/**
 * @brief Configuration macros for bit-banged UART communication and system parameters.
 *
 * These macros define key constants used throughout the bit-banging UART implementation,
 * including pin assignments, baud rates, bit timing, and buffer sizes for both
 * reception and transmission.
 */

#define SERIAL_MONITOR_BAUD_RATE 115200 ///< Baud rate for the standard serial monitor.
#define RX_PIN GPIO_NUM_12              ///< GPIO pin designated for bit-banged UART reception.
#define BAUD_RATE_RX 9600               ///< Baud rate for bit-banged UART reception.
#define BIT_TIME_US_RX (1000000 / BAUD_RATE_RX) ///< Calculated time (in microseconds) for one bit during reception.
#define BUFFER_SIZE 128                 ///< Size of the buffer used for received UART data.

#define BAUD_RATE_TX 9600               ///< Baud rate for bit-banged UART transmission.
#define BIT_TIME_US_TX (1000000 / BAUD_RATE_TX) + 1 ///< Calculated time (in microseconds) for one bit during transmission, with a small adjustment.

#define GPIO_INPUT 0                    ///< Alias for GPIO input mode (for clarity).
#define GPIO_OUTPUT 1                   ///< Alias for GPIO output mode (for clarity).

/**
 * @brief Represents a UART (Universal Asynchronous Receiver-Transmitter) configuration.
 *
 * This structure holds all the necessary parameters to configure and manage
 * a UART peripheral, whether it's a hardware UART or a bit-banged software
 * implementation.
 */
typedef struct {
    int uart_nr;      ///< The UART port number (e.g., 0 for UART0, 1 for UART1).
    int rx_pin;       ///< The GPIO pin number used for receiving data (RX). Note: may be ignored or fixed for certain hardware UARTs (like UART1 on some ESP32s).
    int tx_pin;       ///< The GPIO pin number used for transmitting data (TX).
    int tx_enabled;   ///< A flag (boolean) indicating whether transmission (TX) is enabled for this UART.
    int rx_enabled;   ///< A flag (boolean) indicating whether reception (RX) is enabled for this UART.
    int baud_rate;    ///< The communication speed in bits per second (baud).
} uart_t;

esp_err_t my_uart_init(uart_t *uart);
uint8_t uart_bitbang_receive_byte();
void uart_bitbang_receive_task(void *param);
esp_err_t start_reciving_task(void);
void uart_bitbang_send_string(const char *str, size_t length);
IRAM_ATTR void uart_rx_isr_handler(void *arg);
extern volatile bool start_bit_detected;
extern bool stop_bit;
extern uint8_t received_data[BUFFER_SIZE];
extern uint8_t TX_PIN;

#else
#include "UART_tests.h"
#endif

#endif  // RECIVE_UART_H
