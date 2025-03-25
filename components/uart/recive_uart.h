#ifndef RECIVE_UART_H
#define RECIVE_UART_H

#include "stdio.h"
#include "stdbool.h"
#include "stdarg.h" // Needed for va_list
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp8266/pin_mux_register.h"

#define SERIAL_MONITOR_BAUD_RATE 115200
#define RX_PIN GPIO_NUM_12  // GPIO3 (RX) for UART reception
#define BAUD_RATE 9600
#define BIT_TIME_US (1000000 / BAUD_RATE) // Time per bit in microseconds
#define BUFFER_SIZE 128

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

void my_uart_init(uart_t *uart);
uint8_t uart_bitbang_receive_byte();
void uart_bitbang_receive_task(void *param);
void start_reciving_task();

extern volatile bool start_bit_detected;
extern bool stop_bit;
extern uint8_t received_data[BUFFER_SIZE];

#endif  // RECIVE_UART_H
