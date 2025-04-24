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

#define MAX_FILES 50
#define MAX_STRING_LEN 50

uint8_t received[10];  // Global or large enough buffer
char full_path[160];  // Make sure this is large enough

volatile bool PIRQ_bool = 0;
bool pressed = 0;
uint8_t paragraph_number;

uint16_t* recieve_touch_data(int r) {
	static uint16_t first12 = 0;  // Static to persist after function ends

	if (r <= 0) {
		printf("Invalid value for r\n");
		return NULL;
	}

	for (int i = 0; i < r; i++) {
		received[i] = recive();  // Assume recive() is defined elsewhere
	}


	// Calculate first 12 bits
	if (r >= 2) {
		first12 = ((uint16_t) received[0] << 4) | (received[1] >> 4);
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
		char *case_type, char *previous_task, char *current_task ,TaskParams *data) {

	for (int i = 0; i < 32; i++) { // Loop through all the keys (26 + 1 for DEL or space)
		if ((x >= keyboard[i].x && x <= (keyboard[i].x + keyboard[i].width))
				&& (y >= keyboard[i].y
						&& y <= (keyboard[i].y + keyboard[i].height))) {

			gpio_set_level(SS_display, 0);

			if (strcmp(keyboard[i].label, "close") == 0) {

				clean_screen();

				keyboard_buffer_i = 0;

				while (coord_index_char > 0) {
					clean_last_char();
				}

				if (strcmp(previous_task, "main_menu_task") == 0) {

					xTaskCreate(main_menu_task, "main_menu_task", 2048, NULL, 5,
							&main_menu_Handle);
				} else if (strcmp(current_task, "keyboard_to_edit") == 0) {
					xTaskCreate(notebook_editFilesPage1_task,
							"notebook_editFilesPage1_task", 2048, NULL, 5,
							&other_task_handel);

				} else {
					xTaskCreate(note_book_app_page1, "note_book_app_page1",
							2048, NULL, 5, &other_task_handel);

				}

				gpio_set_level(SS_display, 1);

				free(data);

				vTaskDelete(NULL);

				break;
			}

			if (strcmp(keyboard[i].label, "OK") == 0) {

				if (strcmp(current_task, "keyboard_New_file") == 0) {

					if (keyboard_buffer_i > 10) {

						while (coord_index_char > 10) {
							clean_last_char();
						}

						*x1 = history_char[coord_index_char].x;
						*y1 = history_char[coord_index_char].y;

						keyboard_buffer_i = 9;

						background_color = "red";
						print_ILI9488("name longer than 10 !", 0, 144, 2);
						background_color = "black";
						break;
					}

					clean_screen();

					while (coord_index_char > 0) {
						clean_last_char();
					}

					paragraph_number = 1;

					background_color = "black";

					char temp[keyboard_buffer_i];
					strncpy(temp, keyboard_buffer, keyboard_buffer_i);
					temp[keyboard_buffer_i] = '\n';
					temp[keyboard_buffer_i + 1] = '\0'; // Ensure it's null-terminated

					append_to_file("/spiffs/notebook/filenames.txt", temp);

					char filepath[25]; // Make sure it's big enough to hold the full path

					snprintf(filepath, sizeof(filepath), "/notebook/%s", temp);

					snprintf(full_path, sizeof(full_path), "/spiffs%s",
							filepath);

					append_to_file(full_path, "");

					keyboard_buffer_i = 0;

					print_ILI9488(temp, 100, 5, 2);

					background_color = "black";
					char text[14];
					sprintf(text, "Paragraph %d", paragraph_number);
					print_ILI9488(text, 20, 143, 2);

					background_color = "black";

					TaskParams *params = malloc(sizeof(TaskParams));
					params->y = 35;
					strcpy(params->previous_task, "keyboard_New_file");
					strcpy(params->current_task, "write_textfile_task");

					xTaskCreate(write_textfile_task, "write_textfile_task",
							2048, (void*) params, 5, &other_task_handel);

					free(data);

					vTaskDelete(NULL);
				}

				if (strcmp(previous_task, "keyboard_New_file") == 0) {
					char temp[keyboard_buffer_i];
					strncpy(temp, keyboard_buffer, keyboard_buffer_i);
					temp[keyboard_buffer_i] = '\0'; // Null-terminate the string

					append_to_file(full_path, temp);

					while (coord_index_char > 0) {
						clean_last_char();
					}

					*x1 = 0;
					*y1 = 35;

					keyboard_buffer_i = 0;
					paragraph_number++;
					char text[14];
					sprintf(text, "Paragraph %d", paragraph_number);
					print_ILI9488(text, 20, 144, 2);

					break;
				}

				if (strcmp(current_task, "keyboard_to_edit") == 0) {

					printf("editing lol \n");
					char temp[keyboard_buffer_i];
					strncpy(temp, keyboard_buffer, keyboard_buffer_i);
					temp[keyboard_buffer_i] = '\0'; // Null-terminate the string

					overwrite_file(full_path, temp);

					while (coord_index_char > 0) {
						clean_last_char();
					}

					*x1 = 0;
					*y1 = 35;

					keyboard_buffer_i = 0;
					paragraph_number++;
					char text[14];
					sprintf(text, "Paragraph %d", paragraph_number);
					print_ILI9488(text, 20, 144, 2);

					break;
				}

				gpio_set_level(SS_display, 1);

				free(data);

				vTaskDelete(NULL);

				break;
			}

			if (strcmp(keyboard[i].label, "DEL") == 0) {

				if (coord_index_char == 0 || coord_index_char == 1) {
					break;
				}

				clean_last_char();

				keyboard_buffer_i--;

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
			background_color = "black";
			print_char_ILI9488(c, x1, y1, 2);
			keyboard_buffer[keyboard_buffer_i++] = c;

			send_command(0x00);

			break;  // Exit loop after first key is found
		}
	}
}
