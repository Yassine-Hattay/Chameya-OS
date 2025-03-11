#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "stdio.h"
#include "driver/uart.h"
#include "stdarg.h" // Needed for va_list

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1

// Define UART structure
typedef struct {
    int uart_nr;    // UART number (e.g., UART0, UART1)
    int rx_pin;     // RX pin number (ignored for UART1)
    int tx_pin;     // TX pin number
    int tx_enabled; // Flag indicating if TX is enabled
    int rx_enabled; // Flag indicating if RX is enabled
    int baud_rate;  // Baud rate
} uart_t;

// Manually set the UART pins
void uart_set_pins(int uart_num, int tx_pin, int rx_pin) {
    gpio_set_direction(tx_pin, GPIO_OUTPUT);
    if (rx_pin != -1) { // Only configure RX if provided
        gpio_set_direction(rx_pin, GPIO_INPUT);
    }
}

// Custom UART1 printf handler (vprintf-like function)
int my_uart1_vprintf(const char *fmt, va_list args) {
    char buffer[128];  // Temporary buffer to hold formatted string
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (len > 0) {
        uart_write_bytes(UART_NUM_1, buffer, len);
    }
    return len;
}

// Redirect printf to UART1 (for ESP8266, this is done manually)
int uart1_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[128];
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len > 0) {
        uart_write_bytes(UART_NUM_1, buffer, len);  // Send to UART1
    }

    return len;
}

// Function to initialize UART with a given baud rate
void my_uart_init(uart_t* uart) {
    uart_config_t uart_config = {
        .baud_rate = uart->baud_rate, // Set custom baud rate
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
//plz commit
    uart_param_config(uart->uart_nr, &uart_config);

    // Configure TX pin (no RX for UART1)
    uart_set_pins(uart->uart_nr, uart->tx_pin, uart->rx_enabled ? uart->rx_pin : -1);

    // Install UART driver
    uart_driver_install(uart->uart_nr, 1024, 0, 0, NULL, 0);
}

// Disable UART
void my_uart_uninit(uart_t* uart) {
    if (uart == NULL) return;
    gpio_set_direction(uart->tx_pin, GPIO_INPUT);
    printf("UART %d deinitialized.\n", uart->uart_nr);
}

// app_main: The entry point for ESP8266 application
void app_main() {
    static uart_t uart0 = {0, 3, 1, 1, 1, 115200};  // UART0 (TX: GPIO1, RX: GPIO3) @ 115200 baud
    static uart_t uart1 = {1, -1, 2, 1, 0, 9600};   // UART1 TX-only (TX: GPIO2, No RX) @ 9600 baud

    my_uart_init(&uart0);
    my_uart_init(&uart1);

    // Switch printf to UART0 and send a message
    printf("Hello from UART0 (115200 baud)\n");

    // Switch printf to UART1 and send a message using uart1_printf
    uart1_printf("Hello from UART1 (9600 baud, TX on GPIO2)!\n");

    my_uart_uninit(&uart0);
    my_uart_uninit(&uart1);
}
