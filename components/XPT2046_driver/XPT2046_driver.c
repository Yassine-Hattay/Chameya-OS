/**
 * @file XPT2046_driver.c
 * @author your name (you@domain.com)
 * @brief this is the implementation of the XPT2046 touch controller driver
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "XPT2046_driver.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

uint8_t received[10];  // Global or large enough buffer
char full_path[160];  // Make sure this is large enough

volatile bool PIRQ_bool = 0;
bool pressed = 0;
uint8_t paragraph_number;

/**
 * @brief Receives a specified number of data points and extracts a 12-bit value.
 *
 * This function reads 'r' data points using an assumed `recive()` function,
 * stores them in a `received` array, and then attempts to construct a 12-bit
 * unsigned integer from the first two received bytes.
 *
 * @param r The number of data points to receive. Must be a positive integer.
 * @return A pointer to a static `uint16_t` variable containing the extracted
 * 12-bit value. Returns `NULL` if `r` is invalid, or `0` if there's
 * not enough data to form a 12-bit value.
 */

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
/**
 * @brief Sends a control byte.
 *
 * This function is a wrapper around `send_data` that specifically sends a control byte.
 *
 * @param parameters The 8-bit control byte to be sent.
 */

void send_control_byte(uint8_t parameters) {
	send_data(parameters);
}

/**
 * @brief Initializes the XPT2046 touch controller.
 *
 * This function configures the necessary GPIO pins for communication with an XPT2046
 * touch controller and sends an initial control byte to prepare it for operation.
 * It sets up the MISO and PIRQ pins as inputs, the SS pin as an output,
 * and enables an interrupt on the PIRQ pin for touch detection.
 */

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

/**
 * @brief Checks for key presses on a virtual keyboard and handles corresponding actions.
 * 
 * This function processes touch coordinates to determine which keyboard key was pressed,
 * then performs the appropriate action based on the key and current application context.
 * 
 * @param[in] x The X-coordinate of the touch input.
 * @param[in] y The Y-coordinate of the touch input.
 * @param[in,out] x1 Pointer to the current X position for text display.
 * @param[in,out] y1 Pointer to the current Y position for text display.
 * @param[in,out] case_type Pointer to the current keyboard case (upper/lower/symbol).
 * @param[in] previous_task Name of the previous task/context.
 * @param[in,out] current_task Name of the current task/context.
 * @param[in,out] data Pointer to task parameters structure.
 * 
 * @note Handles special keys (close, OK, DEL, case change), text input, and task switching.
 * Manages various application contexts including file editing, GPIO operations, and navigation.
 */

void check_key_press(uint16_t x, uint16_t y, uint16_t *x1, uint16_t *y1,
		char *case_type, char *previous_task, char *current_task,
		TaskParams *data) {

	for (int i = 0; i < 32; i++) { // Loop through all the keys (26 + 1 for DEL or space)
		if ((x >= keyboard[i].x && x <= (keyboard[i].x + keyboard[i].width))
				&& (y >= keyboard[i].y
						&& y <= (keyboard[i].y + keyboard[i].height))) {

			gpio_set_level(SS_display, 0);

			if (strcmp(keyboard[i].label, "close") == 0) {

				coord_index = 1;

				keyboard_buffer_i = 0;

				FillScreenblack();

				coord_index_char = 1;

				gpio_set_level(SS_display, 1);

				free(data);

				if (strcmp(previous_task, "main_menu_task") == 0) {

					xTaskCreate(main_menu_task, "main_menu_task", 2048, NULL, 5,
							&main_menu_Handle);
				} else if (strcmp(current_task, "keyboard_to_edit") == 0) {
					xTaskCreate(notebook_editFilesPage1_task,
							"notebook_editFilesPage1_task", 2048, NULL, 5,
							&other_task_handel);

				} else if ((strcmp(current_task, "GPIO_C_UART_Transmit") == 0)
						|| (strcmp(current_task, "GPIO_C_I2C_Write") == 0)
						|| (strcmp(current_task, "GPIO_C_I2C_Address") == 0)
						|| (strcmp(current_task, "GPIO_C_SPI_Transmit") == 0)) {

					xTaskCreate(GPIO_C_page_1, "GPIO_C_page_1", 2048, NULL, 5,
							&other_task_handel);

				} else {
					xTaskCreate(note_book_app_page1, "note_book_app_page1",
							2048, NULL, 5, &main_menu_Handle);
				}

				vTaskDelete(NULL);

				break;
			}

			if (strcmp(keyboard[i].label, "OK") == 0) {

				if (strcmp(current_task, "GPIO_C_I2C_Address") == 0) {

					char *endptr;
					unsigned long val = strtol(keyboard_buffer, &endptr, 0);
					I2C_SLAVE_ADDR = (uint8_t) val;

					TaskParams *params = malloc(sizeof(TaskParams));
					params->x = 0;
					params->y = 35;
					strcpy(params->current_task, "GPIO_C_I2C_Write");
					strcpy(current_task, "GPIO_C_I2C_Write");

					gpio_set_level(SS_display, 0);
					clean_screen();

					keyboard_buffer_i = 0;

					while (coord_index_char > 0) {
						clean_last_char();
					}
					coord_index_char = 1;

					if (!Read_write_bit_i2c) {
						print_ILI9488("Send data", 80, 0, 2);
						send_command(0x00);
						gpio_set_level(SS_display, 1);
						xTaskCreate(keyboard_task, "GPIO_C_I2C_Write", 2048,
								(void*) params, 5, &other_task_handel);
						vTaskDelete(NULL);
						break;
					} else {
						send_command(0x00);
						gpio_set_level(SS_display, 1);
					}
				}

				if (Read_write_bit_i2c) {

					uint8_t byte;
					uint8_t j = 0;

					clean_screen();
					uint8_t buffer[BUFFER_SIZE];

					i2c_start();
					i2c_write_byte((I2C_SLAVE_ADDR << 1) | 1);
					one_tick();
					while (j < BUFFER_SIZE - 1) {
						byte = i2c_recive_byte();
						buffer[j++] = byte;

						// Send ACK if not end, NACK if done
						send_ACK_NACK(byte == '\0');

						if (byte == '\0') {
							break;
						}
					}

					buffer[j] = '\0'; // Safety null-terminate
					i2c_stop();

					printf("\n Received: %s\n", buffer);

					gpio_set_level(SS_display, 0);

					print_ILI9488((char*) buffer, 0, 50, 2);

					background_color = "red";
					print_ILI9488("X", 456, 0, 2);

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					*x1 = 500;
					*y1 = 500;

					keyboard_buffer_i = 0;

					break;
				}

				if (strcmp(current_task, "GPIO_C_I2C_Write") == 0) {

					i2c_start();
					i2c_send_byte(I2C_SLAVE_ADDR << 1);
					for (int i = 0; i < keyboard_buffer_i; i++) {
						i2c_send_byte(keyboard_buffer[i]);
					}
					i2c_stop();

					while (coord_index_char > 1) {
						clean_last_char();
					}

					coord_index_char = 1;

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					*x1 = 0;
					*y1 = 35;

					keyboard_buffer_i = 0;

					break;
				}

				if (strcmp(current_task, "keyboard_to_edit") == 0) {

					char temp[keyboard_buffer_i];
					strncpy(temp, keyboard_buffer, keyboard_buffer_i);
					temp[keyboard_buffer_i] = '\0'; // Null-terminate the string

					if (paragraph_number == 1) {
						overwrite_file(full_path, temp);
					} else {
						append_to_file(full_path, temp);
					}

					while (coord_index_char > 1) {
						clean_last_char();
					}

					coord_index_char = 1;

					*x1 = 0;
					*y1 = 35;

					keyboard_buffer_i = 0;
					paragraph_number++;
					char text[14];
					sprintf(text, "Paragraph %d", paragraph_number);
					background_color = "red";

					print_ILI9488(text, 20, 142, 2);

					break;
				}

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

					coord_index_char = 1;

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

					background_color = "red";
					char text[14];
					sprintf(text, "Paragraph %d", paragraph_number);
					print_ILI9488(text, 20, 142, 2);
					send_command(0x00);
					gpio_set_level(SS_display, 1);

					background_color = "black";

					TaskParams *params = malloc(sizeof(TaskParams));
					params->x = 0;
					params->y = 35;
					strcpy(params->previous_task, "keyboard_New_file");
					strcpy(params->current_task, "write_textfile_task");

					free(data);

					xTaskCreate(keyboard_task, "write_textfile_task", 2048,
							(void*) params, 5, &other_task_handel);

					vTaskDelete(NULL);
				}

				if (strcmp(previous_task, "keyboard_New_file") == 0) {
					char temp[keyboard_buffer_i];
					strncpy(temp, keyboard_buffer, keyboard_buffer_i);
					temp[keyboard_buffer_i] = '\0'; // Null-terminate the string

					append_to_file(full_path, temp);

					while (coord_index_char > 1) {
						clean_last_char();

					}

					coord_index_char = 1;

					paragraph_number++;

					background_color = "red";
					char text[14];
					sprintf(text, "Paragraph %d", paragraph_number);
					print_ILI9488(text, 20, 142, 2);
					send_command(0x00);
					gpio_set_level(SS_display, 1);

					*x1 = 0;
					*y1 = 35;

					keyboard_buffer_i = 0;

					break;
				}

				if (strcmp(current_task, "GPIO_C_UART_Transmit") == 0) {
					uart_bitbang_send_string(keyboard_buffer,
							keyboard_buffer_i);

					while (coord_index_char > 1) {
						clean_last_char();
					}

					coord_index_char = 1;

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					*x1 = 0;
					*y1 = 35;

					keyboard_buffer_i = 0;

					break;
				}

				if (strcmp(current_task, "GPIO_C_SPI_Transmit") == 0) {

					spi_master_bit_bang_mode_0(0xA2);

					while (coord_index_char > 1) {
						clean_last_char();
					}

					coord_index_char = 1;

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					*x1 = 0;
					*y1 = 35;

					keyboard_buffer_i = 0;

					break;
				}

				if (strcmp(previous_task, "keyboard_New_file") == 0) {
					char temp[keyboard_buffer_i];
					strncpy(temp, keyboard_buffer, keyboard_buffer_i);
					temp[keyboard_buffer_i] = '\0'; // Null-terminate the string

					append_to_file(full_path, temp);

					while (coord_index_char > 1) {
						clean_last_char();

					}

					coord_index_char = 1;

					paragraph_number++;

					background_color = "red";
					char text[14];
					sprintf(text, "Paragraph %d", paragraph_number);
					print_ILI9488(text, 20, 142, 2);
					send_command(0x00);
					gpio_set_level(SS_display, 1);

					*x1 = 0;
					*y1 = 35;

					keyboard_buffer_i = 0;

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

			break;  // Exit loop after first key is found
		}
	}
}
