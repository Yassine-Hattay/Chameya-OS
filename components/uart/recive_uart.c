#include "recive_uart.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

volatile bool start_bit_detected = 0; // Flag for interrupt
// Define UART structure
bool stop_bit = 0;

void my_uart_init(uart_t *uart) {
	uart_config_t uart_config =
			{
					.baud_rate = uart->baud_rate, // Set custom baud rate
					.data_bits = UART_DATA_8_BITS,
					.parity = UART_PARITY_DISABLE,
					.stop_bits = UART_STOP_BITS_1, .flow_ctrl =
							UART_HW_FLOWCTRL_DISABLE };
	uart_param_config(uart->uart_nr, &uart_config);

	// Install UART driver
	uart_driver_install(uart->uart_nr, 1024, 0, 0, NULL, 0);
}

// ISR to detect the start of the UART reception (start bit)
static IRAM_ATTR void uart_rx_isr_handler(void *arg) {
		start_bit_detected = 1;
}

// Function to receive a string and store it in a buffer using bit-banging
void uart_bitbang_receive_task(void *param) {
	uint8_t received_data[BUFFER_SIZE];
	int index = 0;
	while (1) {

		if (start_bit_detected) {
			uint8_t byte = 0;

			gpio_isr_handler_remove(RX_PIN); // Disable interrupt while receiving

			ets_delay_us(BIT_TIME_US / 2);  // Move to center of first data bit

			for (int i = 0; i < 8; i++) {
				ets_delay_us(BIT_TIME_US);  // Wait for each bit
				byte |= (gpio_get_level(RX_PIN) << i); // Read bit and store in byte
			}
			ets_delay_us(BIT_TIME_US);  // Wait for each bit
			stop_bit = gpio_get_level(RX_PIN);

			if (!stop_bit) {
				ets_delay_us(BIT_TIME_US * 1.1);
				index = 0;
			} else {
				if (index < BUFFER_SIZE) {
					received_data[index++] = byte;
				} else {
					printf("%s", received_data);
					index = 0;
					esp_task_wdt_reset();
				}
				stop_bit = 0;

			}
			start_bit_detected = 0;
			gpio_isr_handler_add(RX_PIN, uart_rx_isr_handler, NULL);
		}

		}
}

void start_reciving_task() {
	gpio_config_t io_conf = { .pin_bit_mask = (1ULL << RX_PIN), .mode =
			GPIO_MODE_INPUT,  // Allows both read and write
			.pull_up_en = GPIO_PULLUP_ENABLE, .pull_down_en =
					GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_NEGEDGE };
	gpio_config(&io_conf);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(RX_PIN, uart_rx_isr_handler, NULL);

	xTaskCreate(uart_bitbang_receive_task, "uart_rx_task", 4096, NULL, configMAX_PRIORITIES - 1 , NULL);

}

