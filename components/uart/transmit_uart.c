#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"

// UART Configuration
#define TX_PIN GPIO_NUM_2  // GPIO2 (D4) for UART1 TX
#define BAUD_RATE 115200
#define BIT_TIME_US (1000000 / BAUD_RATE) // Time per bit in microseconds

// Function to send a single byte via bit-banging
void uart_bitbang_send_byte(uint8_t byte) {
    // Start bit (low)
    gpio_set_level(TX_PIN, 0);
    ets_delay_us(BIT_TIME_US);

    // Send 8 data bits (LSB first)
    for (int i = 0; i < 8; i++) {
        gpio_set_level(TX_PIN, (byte >> i) & 1);
        ets_delay_us(BIT_TIME_US);
    }

    // Stop bit (high)
    gpio_set_level(TX_PIN, 1);
    ets_delay_us(BIT_TIME_US);
}

// Function to send a string
void uart_bitbang_send_string(const char *str) {
    while (*str) {
        uart_bitbang_send_byte(*str++);
    }
}

// Task to continuously send a message
void uart_task(void *param) {
    while (1) {
        uart_bitbang_send_string("Hello from bit-banged UART1 on GPIO2 (D4)!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second
    }
}

// Main function
void app_main() {
    // Configure TX pin as output
    gpio_set_direction(TX_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TX_PIN, 1);  // Idle state is high

    // Start UART task
    xTaskCreate(uart_task, "uart_task", 2048, NULL, 1, NULL);
}
