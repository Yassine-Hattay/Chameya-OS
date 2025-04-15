/*
 * tasks.c
 *
 *  Created on: 15 Apr 2025
 *      Author: hatta
 */

#include "tasks.h"

TaskHandle_t main_menu_Handle = NULL;
TaskHandle_t keyboard_task_Handle = NULL;


void IRAM_ATTR PIRQ_isr_handler(void *arg) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	// Check if the task handle is valid before notifying
	if (main_menu_Handle != NULL) {
		vTaskNotifyGiveFromISR(main_menu_Handle, &xHigherPriorityTaskWoken);
	}
	// Check if the task handle is valid before notifying
	if (keyboard_task_Handle != NULL) {
		vTaskNotifyGiveFromISR(keyboard_task_Handle, &xHigherPriorityTaskWoken);
	}

	portYIELD_FROM_ISR();

}


void main_menu_task(void *pvParameters) {

	float scaling_factor = 1.013; // Adjust this for the desired speed of growth
	float cell_size = 1.875;
	float cell_size1 = 1.25;

	while (1) {
		printf("main_menu_task \n");
		// Wait indefinitely for ISR to notify us
		vTaskDelay(100);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		// Touch IRQ has been triggered
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
			continue;
		}

		float x_float = pow((255 - first8_msb), scaling_factor) * cell_size;
		uint16_t x = (uint16_t) roundf(x_float);

		float y_float = pow((255 - first8_msb1), scaling_factor) * cell_size1;
		uint16_t y = (uint16_t) roundf(y_float);

		printf("Mapped Coordinates: x = %u, y = %u\n", x, y);

		if ((x >= history[coord_index - 1].x
				&& x
						<= (history[coord_index - 1].x
								+ history[coord_index - 1].width))
				&& (y >= history[coord_index - 1].y
						&& y
								<= (history[coord_index - 1].y
										+ history[coord_index - 1].height))
				&& strcmp(history[coord_index - 1].app_name, "notebook") == 0) { // Compare app_name to "notebook"
			gpio_set_level(SS_display, 0);
			clean_screen();
			draw_keyborad('l');
			xTaskCreate(note_book_task, "note_book_task", 2048, NULL, 5,
					&keyboard_task_Handle);
			send_command(0x00);
			gpio_set_level(SS_display, 1);

			vTaskDelete(NULL);

		}

	}
}



void note_book_task(void *pvParameters) {

	uint16_t x1 = 0;
	uint16_t y1 = 0;
	char case_type = 'l';
	float scaling_factor = 1.013; // Adjust this for the desired speed of growth
	float cell_size = 1.875;
	float cell_size1 = 1.25;

	while (1) {
		printf("note_book_task \n");

		// Wait indefinitely for ISR to notify us
		vTaskDelay(100);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		// Touch IRQ has been triggered
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
			continue;
		}

		float x_float = pow((255 - first8_msb), scaling_factor) * cell_size;
		uint16_t x = (uint16_t) roundf(x_float);

		float y_float = pow((255 - first8_msb1), scaling_factor) * cell_size1;
		uint16_t y = (uint16_t) roundf(y_float);

		printf("Mapped Coordinates: x = %u, y = %u\n", x, y);
		check_key_press(x, y, &x1, &y1, &case_type);
		send_command(0x00);
		gpio_set_level(SS_display, 1);
	}

}


