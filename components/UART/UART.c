#include "UART.h"


volatile bool start_bit_detected = 0; // Flag for interrupt
// Define UART structure
bool stop_bit = 0;
uint8_t TX_PIN = 2;

esp_err_t my_uart_init(uart_t *uart) {
    uart_config_t uart_config = {
        .baud_rate = uart->baud_rate, // Set custom baud rate
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Configure UART parameters
    esp_err_t err = uart_param_config(uart->uart_nr, &uart_config);
    if (err != ESP_OK) {
        return err; // Return the error if configuration fails
    }

    // Install UART driver
    err = uart_driver_install(uart->uart_nr, 1024, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        return err; // Return the error if driver installation fails
    }

    return ESP_OK; // Return success if both operations succeed
}


// ISR to detect the start of the UART reception (start bit)
IRAM_ATTR void uart_rx_isr_handler(void *arg) {
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

			ets_delay_us(BIT_TIME_US_RX / 2); // Move to center of first data bit

			for (int i = 0; i < 8; i++) {
				ets_delay_us(BIT_TIME_US_RX);  // Wait for each bit
				byte |= (gpio_get_level(RX_PIN) << i); // Read bit and store in byte
			}
			ets_delay_us(BIT_TIME_US_RX);  // Wait for each bit
			stop_bit = gpio_get_level(RX_PIN);

			if (!stop_bit) {
				ets_delay_us(BIT_TIME_US_RX * 1.1);
				index = 0;
			} else {
				if (index < BUFFER_SIZE) {
					received_data[index++] = byte;
				} else {
					printf("new : %s \n", received_data);
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

esp_err_t start_reciving_task(void) {
    gpio_config_t io_conf = { .pin_bit_mask = (1ULL << RX_PIN),
                              .mode = GPIO_MODE_INPUT,
                              .pull_up_en = GPIO_PULLUP_ENABLE,
                              .pull_down_en = GPIO_PULLDOWN_DISABLE,
                              .intr_type = GPIO_INTR_NEGEDGE };

    // GPIO configuration
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        printf("start_reciving_task: GPIO configuration failed, error: %d\n", ret);
        return ret;  // Return error code if GPIO config fails
    }

    // Install ISR service
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK) {
        printf("start_reciving_task: ISR service installation failed, error: %d\n", ret);
        return ret;  // Return error code if ISR service fails
    }

    // Add ISR handler
    ret = gpio_isr_handler_add(RX_PIN, uart_rx_isr_handler, NULL);
    if (ret != ESP_OK) {
        printf("start_reciving_task: ISR handler addition failed, error: %d\n", ret);
        return ret;  // Return error code if adding ISR handler fails
    }

    // Create receiving task
    BaseType_t task_create_status = xTaskCreate(uart_bitbang_receive_task,
                                                   "uart_rx_task",
                                                   4096,
                                                   NULL,
                                                   configMAX_PRIORITIES - 1,
                                                   NULL);
    if (task_create_status != pdPASS) {
        printf("start_reciving_task: Task creation failed\n");
        return ESP_FAIL;  // Return failure code if task creation fails
    }

    return ESP_OK;  // Return success if everything succeeded
}


// Function to send a single byte via bit-banging
void uart_bitbang_send_byte(uint8_t byte) {
	// Start bit (low)
	gpio_set_level(TX_PIN, 0);
	ets_delay_us(BIT_TIME_US_TX);

	// Send 8 data bits (LSB first)
	for (int i = 0; i < 8; i++) {
		gpio_set_level(TX_PIN, (byte >> i) & 1);
		ets_delay_us(BIT_TIME_US_TX);
	}

	// Stop bit (high)
	gpio_set_level(TX_PIN, 1);
	ets_delay_us(BIT_TIME_US_TX);
}

// Function to send a string
void uart_bitbang_send_string(const char *str, size_t length) {
	for (size_t i = 0; i < length; i++) {
		uart_bitbang_send_byte(str[i]);
	}
}

// Task to continuously send a message
void uart_task(void *param) {
	while (1) {
		//uart_bitbang_send_string(
		//		"Hello from bit-banged UART1 on GPIO2 (D4)!\n");
		//vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second
	}
}

// Main function
void init_transmit_task() {
	// Configure TX pin as output
	gpio_set_direction(TX_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(TX_PIN, 1);  // Idle state is high

	// Start UART task
	xTaskCreate(uart_task, "uart_task", 2048, NULL, 1, NULL);
}
