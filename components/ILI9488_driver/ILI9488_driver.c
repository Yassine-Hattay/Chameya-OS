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

	send_command(0x3A);  // set pixel format
	send_ILI9488_data(0x06);
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

void set_resolution_pos(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	uint16_t x_end = x + width - 1;
	uint16_t y_end = y + height - 1;

	// Split into high and low bytes
	uint8_t x_start_high = (x >> 8) & 0xFF;
	uint8_t x_start_low = x & 0xFF;
	uint8_t x_end_high = (x_end >> 8) & 0xFF;
	uint8_t x_end_low = x_end & 0xFF;

	uint8_t y_start_high = (y >> 8) & 0xFF;
	uint8_t y_start_low = y & 0xFF;
	uint8_t y_end_high = (y_end >> 8) & 0xFF;
	uint8_t y_end_low = y_end & 0xFF;

	printf("Setting column and page address!\n");

	send_command(0x2A); // Column Address Set
	send_ILI9488_data(x_start_high);
	send_ILI9488_data(x_start_low);
	send_ILI9488_data(x_end_high);
	send_ILI9488_data(x_end_low);

	send_command(0x2B); // Page Address Set
	send_ILI9488_data(y_start_high);
	send_ILI9488_data(y_start_low);
	send_ILI9488_data(y_end_high);
	send_ILI9488_data(y_end_low);

}

void set_orientation(uint8_t orientation) {

	send_command(0x36); // Memory Access Control

	if (orientation == 0) {
		send_ILI9488_data(0x08);
	} else if (orientation == 1) {
		send_ILI9488_data(0x28);
	} else if (orientation == 2) {
		send_ILI9488_data(0x48);

	} else if (orientation == 3) {
		send_ILI9488_data(0x68);

	}
	if (orientation == 4) {
		send_ILI9488_data(0x88);
	} else if (orientation == 5) {
		send_ILI9488_data(0xA8);
	} else if (orientation == 6) {
		send_ILI9488_data(0xC8);

	} else if (orientation == 7) {
		send_ILI9488_data(0xE8);

	} else {
		printf("Error : orientation must be between [0,3] ");
	}

}

void mirror_image() {
}
