#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define MOSI 13
#define MISO 12
#define SCK  14
#define SS   15


// Full-duplex SPI: Send and receive at the same time
uint8_t spi_bit_bang_mode_0(uint8_t data_to_send) {
	uint8_t received = 0;

	// Pull CS low to start communication
	gpio_set_level(SS, 0);
	ets_delay_us(10);

	for (int i = 7; i >= 0; i--) {
		// Set MOSI
		gpio_set_level(MOSI, (data_to_send >> i) & 1);
		ets_delay_us(10);

		gpio_set_level(SCK, 1);
		ets_delay_us(10);

		received |= (gpio_get_level(MISO) << i);
		gpio_set_level(SCK, 0);

	}

	// Pull CS high to end communication
	gpio_set_level(SS, 1);

	// Return received data
	return received;
}



// Full-duplex SPI: Send and receive at the same time
uint8_t spi_bit_bang_mode_1(uint8_t data_to_send) {
	uint8_t received = 0;

	// Pull CS low to start communication
	gpio_set_level(SS, 0);
	ets_delay_us(10);

	for (int i = 7; i >= 0; i--) {
		// Set MOSI
		gpio_set_level(MOSI, (data_to_send >> i) & 1);
		gpio_set_level(SCK, 1);
		ets_delay_us(10);
		gpio_set_level(SCK, 0);
		ets_delay_us(10);
		received |= (gpio_get_level(MISO) << i);

	}

	// Pull CS high to end communication
	gpio_set_level(SS, 1);

	// Return received data
	return received;
}
void spi_task(void *pvParameter) {
	uint8_t data_to_send = 0xAA;  // Example data
	uint8_t received_data = 0;

	while (1) {
		received_data = spi_bit_bang_mode_1(data_to_send);
		printf("Received: 0x%02X\n", received_data);

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void user_init(void) {
	// Configure GPIO
	gpio_config_t io_conf = { .pin_bit_mask = (1ULL << MOSI) | (1ULL << SCK)
			| (1ULL << SS), .mode = GPIO_MODE_OUTPUT, .pull_up_en =
			GPIO_PULLUP_DISABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type = GPIO_INTR_DISABLE };
	gpio_config(&io_conf);

	// Configure MISO pin as input
	gpio_config_t io_conf_slave = { .pin_bit_mask = (1ULL << MISO), .mode =
			GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
			GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE };
	gpio_config(&io_conf_slave);
	// Default pin states
	gpio_set_level(MOSI, 0);
	gpio_set_level(SCK, 0);
	gpio_set_level(SS, 1);

	xTaskCreate(spi_task, "spi_task", 1024, NULL, 1, NULL);
}
