/*
 * ILI9488_driver.c
 *
 *  Created on: 5 Apr 2025
 *      Author: hatta
 */

#include "ILI9488_driver.h"

uint8_t received[];

void send_data(uint8_t data_to_send) {

	gpio_set_level(SCK, 0);

	for (int i = 7; i >= 0; i--) {
		// Set MOSI
		gpio_set_level(MOSI, (data_to_send >> i) & 1);
		gpio_set_level(SCK, 1);
		gpio_set_level(SCK, 0);

	}

}

void tick_spi_ILI9488() {
	gpio_set_level(SCK, 0);
	gpio_set_level(SCK, 1);
}

uint8_t recive() {
	uint8_t received = 0;
	gpio_set_level(SCK, 0);

	for (int i = 7; i >= 0; i--) {

		gpio_set_level(SCK, 1);

		received |= (gpio_get_level(MISO) << i);
		gpio_set_level(SCK, 0);
	}

	// Return received data
	return received;
}

void hardware_reset() {

	gpio_set_level(RESET_pin, 1);
	vTaskDelay(15);
	gpio_set_level(RESET_pin, 0);
	vTaskDelay(15);
	gpio_set_level(RESET_pin, 1);

}

void send_command(uint8_t command) {
	if (gpio_get_level(DC_pin)) {
		gpio_set_level(DC_pin, 0);

	}
	send_data(command);

}

void send_ILI9488_data(uint8_t data) {
	if (!gpio_get_level(DC_pin)) {
		gpio_set_level(DC_pin, 1);

	}
	send_data(data);
}

void init_display() {
	gpio_config_t io_conf = { .pin_bit_mask = (1ULL << DC_pin) | (1ULL << SS)
			| (1ULL << RESET_pin), .mode = GPIO_MODE_OUTPUT, .pull_up_en =
			GPIO_PULLUP_DISABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type = GPIO_INTR_DISABLE };
	gpio_config(&io_conf);

	hardware_reset();

	spi_master_init();

	gpio_set_level(SS, 0);
	gpio_set_level(DC_pin, 0);

	send_command(0x11);  // Sleep OUT
	send_command(0x13); //  Normal Display Mode ON
	send_command(0x29); // Display ON

}

uint8_t* recieve_data(int r) {

	if (r <= 0) {
		printf("Invalid value for r\n");
		return NULL;
	}

	if (!gpio_get_level(DC_pin)) {
		gpio_set_level(DC_pin, 1);
	}

	for (int i = 0; i < r; i++) {
		received[i] = recive();  // You must have a `recive()` function
	}

	for (int j = 0; j < r; j++) {
		printf("received%d: ", j + 1);
		for (int i = 7; i >= 0; i--) {
			printf("%d", (received[j] >> i) & 1);
		}
		printf("\n");
	}

	return received;
}




