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

#define SERIAL_MONITOR_BAUD_RATE 115200
#define RX_PIN GPIO_NUM_12  // GPIO3 (RX) for UART reception
#define BAUD_RATE_RX 9600
#define BIT_TIME_US_RX (1000000 / BAUD_RATE_RX) // Time per bit in microseconds
#define BUFFER_SIZE 128

#define BAUD_RATE_TX 9600
#define BIT_TIME_US_TX (1000000 / BAUD_RATE_TX) + 1// Time per bit in microseconds

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1

typedef struct {
	int uart_nr;    // UART number (e.g., UART0, UART1)
	int rx_pin;     // RX pin number (ignored for UART1)
	int tx_pin;     // TX pin number
	int tx_enabled; // Flag indicating if TX is enabled
	int rx_enabled; // Flag indicating if RX is enabled
	int baud_rate;  // Baud rate
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
