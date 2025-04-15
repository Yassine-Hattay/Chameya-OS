/*
 * XPT2046_driver.c
 *
 *  Created on: 11 Apr 2025
 *      Author: hatta
 */

#include "XPT2046_driver.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

uint8_t received[10];  // Global or large enough buffer

volatile bool PIRQ_bool = 0;
bool pressed = false;

uint16_t* recieve_touch_data(int r) {
	static uint16_t first12 = 0;  // Static to persist after function ends

	if (r <= 0) {
		printf("Invalid value for r\n");
		return NULL;
	}

	for (int i = 0; i < r; i++) {
		received[i] = recive();  // Assume recive() is defined elsewhere
	}

	// Print received bytes in binary (optional)
	for (int j = 0; j < r; j++) {
		printf("received%d: ", j + 1);
		for (int i = 7; i >= 0; i--) {
			printf("%d", (received[j] >> i) & 1);
		}
		printf("\n");
	}

	// Calculate first 12 bits
	if (r >= 2) {
		first12 = ((uint16_t) received[0] << 4) | (received[1] >> 4);
		printf("First 12 bits (hex): %03X\n", first12);
	} else {
		printf("Not enough data for 12 bits\n");
		first12 = 0;
	}

	return &first12;
}

void send_control_byte(uint8_t parameters) {
	send_data(parameters);
}





void draw_IRQ() {
	while (1) {
		if (PIRQ_bool == 1) {
			PIRQ_bool = 0;
			gpio_set_level(SS_touch, 0);
			send_control_byte(0x98);
			tick_spi();
			uint8_t first8_msb = (uint8_t) (*recieve_touch_data(2) >> 4);
			gpio_set_level(SS_touch, 1);
			gpio_set_level(SS_touch, 0);
			send_control_byte(0xD8);
			tick_spi();
			uint8_t first8_msb1 = (uint8_t) (*recieve_touch_data(2) >> 4);

			gpio_set_level(SS_touch, 1);

			if (first8_msb1 == 0 && first8_msb == 255) {
				PIRQ_bool = 0;
				continue;
			}

			float cell_size = 1.9;
			float cell_size1 = 1.25;

			float y_float = (255 - first8_msb1) * cell_size1;
			float x_float = (255 - first8_msb) * cell_size;

			uint16_t x = (uint16_t) roundf(x_float);
			uint16_t y = (uint16_t) roundf(y_float);

			if (y > 292) {
				y = 292;
			}

			if (x > 456) {
				x = 456;
			}

			printf("Mapped Coordinates: x = %u, y = %u\n", x, y);

			gpio_set_level(SS_display, 0);
			print_ILI9488("Z", x, y, 2);
			gpio_set_level(SS_display, 1);

		}
	}
}

void init_XPT2046() {
	gpio_config_t io_conf0 = { .pin_bit_mask = (1ULL << MISO_touch), .mode =
			GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
			GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE };

	gpio_config(&io_conf0);

	gpio_config_t io_conf1 = { .pin_bit_mask = (1ULL << SS_touch), .mode =
			GPIO_MODE_OUTPUT, .pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
			GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE };
	gpio_config(&io_conf1);

	gpio_config_t io_conf2 = { .pin_bit_mask = (1ULL << PIRQ_pin), .mode =
			GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
			GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_NEGEDGE };
	gpio_config(&io_conf2);

	gpio_set_level(SS_touch, 0);

	send_control_byte(0x98);

	gpio_set_level(SS_touch, 1);

}



void check_key_press(uint16_t x, uint16_t y, uint16_t *x1, uint16_t *y1,
		char *case_type) {

	for (int i = 0; i < 31; i++) { // Loop through all the keys (26 + 1 for DEL or space)
		if ((x >= keyboard[i].x && x <= (keyboard[i].x + keyboard[i].width))
				&& (y >= keyboard[i].y
						&& y <= (keyboard[i].y + keyboard[i].height))) {

			gpio_set_level(SS_display, 0);

			if (strcmp(keyboard[i].label, "close") == 0) {

				clean_screen();

				while (coord_index_char > 0) {
					clean_last_char();
				}

				draw_apps_icons();

				xTaskCreate(main_menu_task, "main_menu_task", 2048, NULL, 5,
						&main_menu_Handle);

				gpio_set_level(SS_display, 1);

				vTaskDelete(NULL);

				break;
			}
			if (strcmp(keyboard[i].label, "DEL") == 0) {

				if (coord_index_char == 0) {
					break;
				}

				clean_last_char();

				if (*x1 - history_char[coord_index_char].width
						+ history_char[coord_index_char].correction < 0) {
					*y1 = *y1 - history_char[coord_index_char].height;
					*x1 = history_char[coord_index_char].x;
				} else {
					*x1 = *x1 - history_char[coord_index_char].width
							+ history_char[coord_index_char].correction;
				}

				break;
			}

			if (strcmp(keyboard[i].label, "~") == 0) {

				if (*case_type == 'u') {
					*case_type = 'l';
				} else {
					*case_type = 'u';
				}

				draw_keyborad(*case_type);

				break;
			}

			if (strcmp(keyboard[i].label, "?12") == 0) {

				if (pressed == 1) {
					draw_keyborad(*case_type);
					pressed = 0;
					break;

				}

				pressed = 1;
				draw_keyborad('s');

				break;
			}

			char c = keyboard[i].label[0];
			print_char_ILI9488(c, x1, y1, 2);
			send_command(0x00);

			break;  // Exit loop after first key is found
		}
	}
}



