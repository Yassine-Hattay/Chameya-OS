/*
 * tasks.c
 *
 *  Created on: 15 Apr 2025
 *      Author: hatta
 */

#include "tasks.h"

#define MAX_FILES 100
#define MAX_FILENAME_LEN 64

float scaling_factor = 1.013; // Adjust this for the desired speed of growth
float cell_size = 1.875;
float cell_size1 = 1.25;

TaskHandle_t main_menu_Handle = NULL;
TaskHandle_t other_task_handel = NULL;

char keyboard_buffer[MAX_COORDS_CHAR];
uint8_t keyboard_buffer_i = 0;

// Touch regions for UP and DOWN
#define BUTTON_UP_X_START 0
#define BUTTON_UP_X_END 240
#define BUTTON_UP_Y_START 0
#define BUTTON_UP_Y_END 100

#define BUTTON_DOWN_X_START 0
#define BUTTON_DOWN_X_END 240
#define BUTTON_DOWN_Y_START 220
#define BUTTON_DOWN_Y_END 320

// Positions and sizes
int paddle_width = 10;
int paddle_height = 50;

int left_paddle_x = 10;
int left_paddle_y = 120;

int right_paddle_x = 460;  // 480 - paddle_width - some margin
int right_paddle_y = 120;

int ball_x = 240;
int ball_y = 160;
int ball_size = 15;

void IRAM_ATTR PIRQ_isr_handler(void *arg) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	// Check if the task handle is valid before notifying
	if (main_menu_Handle != NULL) {
		vTaskNotifyGiveFromISR(main_menu_Handle, &xHigherPriorityTaskWoken);
	}
	// Check if the task handle is valid before notifying
	if (other_task_handel != NULL) {
		vTaskNotifyGiveFromISR(other_task_handel, &xHigherPriorityTaskWoken);
	}

	portYIELD_FROM_ISR();

}

bool calculate_x_y(uint16_t *x, uint16_t *y) { // Wait indefinitely for ISR to notify us
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
		return 0;
	}

	float x_float = pow((255 - first8_msb), scaling_factor) * cell_size;
	*x = (uint16_t) roundf(x_float);

	float y_float = pow((255 - first8_msb1), scaling_factor) * cell_size1;
	*y = (uint16_t) roundf(y_float);

	return 1;
}

// --- PONG TASK ---

// Helper: Fill rectangle with black or white
void fill_rect(int x, int y, int width, int height, uint8_t color_byte) {

	set_resolution_pos(x, y, width, height, 0);

	send_command(0x2C);
	int total_pixels = width * height;
	for (int i = 0; i < total_pixels / 2; i++) {
		send_ILI9488_data(color_byte);
	}
	send_command(0x00);

}

void draw_paddle(int x, int y) {
	fill_rect(x, y, paddle_width, paddle_height, 0xFF);  // White paddle
}

void draw_ball(int x, int y) {
	fill_rect(x, y, ball_size, ball_size, 0xFF);  // White ball
}

float clampf(float val, float min, float max) {
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}

void memory_monitor_task(void *pvParameters) {
	while (1) {
		printf("Current free heap: %u bytes\n", esp_get_free_heap_size());
		vTaskDelay(500);  // Every 5 seconds
	}
}

void bootApp_pong_B() {

	uint16_t width = 24;
	uint16_t height = 29;
	uint16_t x = 456;
	uint16_t y = 0;

	gpio_set_level(SS_display, 0);

	strcpy(history_char[coord_index_char].app_name, "close");
	history_char[coord_index_char].x = x;
	history_char[coord_index_char].y = y;
	history_char[coord_index_char].width = width;
	history_char[coord_index_char].height = height;

	coord_index_char++;

	background_color = "red";
	print_ILI9488("X", 456, 0, 2);

	background_color = "red";

	height = 70;
	x = 70;

	make_button("Easy", height, x, 30);

	make_button("Medium", height, x, 120);

	make_button("Chameya Mabloula!", height, x, 210);

	send_command(0x00);

	gpio_set_level(SS_display, 1);

}

void bootApp_pong() {

	uint16_t width = 24;
	uint16_t height = 29;
	uint16_t x = 456;
	uint16_t y = 0;

	gpio_set_level(SS_display, 0);

	strcpy(history_char[coord_index_char].app_name, "close");
	history_char[coord_index_char].x = x;
	history_char[coord_index_char].y = y;
	history_char[coord_index_char].width = width;
	history_char[coord_index_char].height = height;

	coord_index_char++;

	background_color = "red";
	print_ILI9488("X", 456, 0, 2);

	background_color = "red";

	height = 70;
	x = 70;

	make_button("PvP", height, x, 30);

	make_button("AI vs AI", height, x, 120);

	make_button("Player vs AI", height, x, 210);

	send_command(0x00);

	gpio_set_level(SS_display, 1);

}

void pong_gamePvP_task(void *pvParameters) {

	int default_ball_speed_x = 10;   // Ball speed horizontally
	int default_ball_speed_y = 6;
	int paddle_speed = 4;           // Default paddle speed (will be updated)

	coord_index_char = 1;
	bootApp_pong_B();

	bool break_v = 1;
	while (break_v) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= 4; i++) {
			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {

				gpio_set_level(SS_display, 0);
				clean_screen();
				send_command(0x00);
				gpio_set_level(SS_display, 1);

				if (strcmp(history_char[coord_index_char - i].app_name, "close")
						== 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task",
							2048, NULL, 5, &main_menu_Handle);
					vTaskDelete(NULL);
				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Easy") == 0) {
					default_ball_speed_x = 10;
					default_ball_speed_y = 6;
					paddle_speed = 4;
					break_v = 0;
					break;
				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Medium") == 0) {
					default_ball_speed_x = 15;
					default_ball_speed_y = 9;
					paddle_speed = 6;

					break_v = 0;
					break;
				} else {
					// Chameya Mabloula mode
					default_ball_speed_x = 20;
					default_ball_speed_y = 12;
					paddle_speed = 7;  // Faster paddle movement
					break_v = 0;
					break;
				}
			}
		}
	}

	float ball_speed_x = default_ball_speed_x;
	float ball_speed_y = default_ball_speed_y;
	coord_index_char = 1;

	char score_str[16];
	int left_score = 0;
	int right_score = 0;

	gpio_set_level(SS_display, 0);
	sprintf(score_str, "%d-%d", left_score, right_score);
	background_color = "black";
	print_ILI9488(score_str, 180, 0, 2);
	strcpy(history_char[coord_index_char].app_name, "close");
	history_char[coord_index_char].x = 305;
	history_char[coord_index_char].y = 0;
	history_char[coord_index_char].width = 24;
	history_char[coord_index_char].height = 29;
	coord_index_char++;
	background_color = "red";
	print_ILI9488("X", 290, 0, 2);
	gpio_set_level(SS_display, 1);

	float last_ball_x = ball_x;
	float last_ball_y = ball_y;
	int last_left_paddle_y = left_paddle_y;
	int last_right_paddle_y = right_paddle_y;

	srand((unsigned int) esp_timer_get_time());

	while (1) {
		uint16_t x_touch, y_touch;

		last_left_paddle_y = left_paddle_y;
		last_right_paddle_y = right_paddle_y;
		last_ball_x = ball_x;
		last_ball_y = ball_y;

		// Touch input
		if (calculate_x_y(&x_touch, &y_touch)) {
			if (x_touch < 240) {
				if (y_touch < 160)
					left_paddle_y -= paddle_speed;
				else if (y_touch > 160)
					left_paddle_y += paddle_speed;
			} else {
				if (y_touch < 160)
					right_paddle_y -= paddle_speed;
				else if (y_touch > 160)
					right_paddle_y += paddle_speed;
			}
		}

		if (x_touch >= 305 && x_touch <= 329 && y_touch <= 29) {

			gpio_set_level(SS_display, 0);
			clean_screen();
			fill_rect(last_ball_x, last_ball_y, ball_size, ball_size, 0x00);
			fill_rect(ball_x, ball_y, ball_size, ball_size, 0x00);
			fill_rect(0, 0, 25, 320, 0x00);
			fill_rect(460, 0, 25, 320, 0x00);
			fill_rect(0, 0, 480, 30, 0x00);

			send_command(0x00);
			gpio_set_level(SS_display, 1);

			xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task", 2048, NULL,
					5, &main_menu_Handle);

			vTaskDelete(NULL);
		}

		// Clamp paddles
		if (left_paddle_y < 0)
			left_paddle_y = 0;
		if (left_paddle_y > 320 - paddle_height)
			left_paddle_y = 320 - paddle_height;
		if (right_paddle_y < 0)
			right_paddle_y = 0;
		if (right_paddle_y > 320 - paddle_height)
			right_paddle_y = 320 - paddle_height;

		// Ball wall collision
		if (ball_y < 0 || ball_y >= 320 - ball_size)
			ball_speed_y = -ball_speed_y;

		// Ball movement
		ball_x += ball_speed_x;
		ball_y += ball_speed_y;

		// Left paddle collision
		if (ball_x <= left_paddle_x + paddle_width
				&& ball_y + ball_size >= left_paddle_y
				&& ball_y <= left_paddle_y + paddle_height) {

			float relative_intersect_y = (left_paddle_y + paddle_height / 2.0f)
					- (ball_y + ball_size / 2.0f);
			float normalized = clampf(
					relative_intersect_y / (paddle_height / 2.0f), -1.0f, 1.0f);
			float bounce_angle = normalized * (3.14159f / 3);

			float speed = sqrtf(
					ball_speed_x * ball_speed_x + ball_speed_y * ball_speed_y);
			ball_speed_x = (speed * cosf(bounce_angle));
			ball_speed_y = (-speed * sinf(bounce_angle));
			if (ball_speed_x == 0)
				ball_speed_x = default_ball_speed_x;
		}

		// Right paddle collision
		if (ball_x >= right_paddle_x - ball_size
				&& ball_y + ball_size >= right_paddle_y
				&& ball_y <= right_paddle_y + paddle_height) {

			float relative_intersect_y = (right_paddle_y + paddle_height / 2.0f)
					- (ball_y + ball_size / 2.0f);
			float normalized = clampf(
					relative_intersect_y / (paddle_height / 2.0f), -1.0f, 1.0f);
			float bounce_angle = normalized * (3.14159f / 3);

			float speed = sqrtf(
					ball_speed_x * ball_speed_x + ball_speed_y * ball_speed_y);
			ball_speed_x = (-speed * cosf(bounce_angle));
			ball_speed_y = (-speed * sinf(bounce_angle));
			if (ball_speed_x == 0)
				ball_speed_x = -default_ball_speed_x;
		}

		// Scoring
		if (ball_x >= 480) {
			left_score++;
			ball_x = 240;
			ball_y = 160;
			ball_speed_x = -1 * default_ball_speed_x;
			ball_speed_y = 0;

			sprintf(score_str, "%d-%d", left_score, right_score);
			gpio_set_level(SS_display, 0);
			background_color = "black";
			print_ILI9488(score_str, 180, 0, 2);
			gpio_set_level(SS_display, 1);
		}
		if (ball_x <= 0) {
			right_score++;
			ball_x = 240;
			ball_y = 160;
			ball_speed_x = default_ball_speed_x;
			ball_speed_y = 0;
			sprintf(score_str, "%d-%d", left_score, right_score);
			gpio_set_level(SS_display, 0);
			print_ILI9488(score_str, 180, 0, 2);
			gpio_set_level(SS_display, 1);
		}

		gpio_set_level(SS_display, 0);

		// Clear previous
		fill_rect(last_ball_x, last_ball_y, ball_size, ball_size, 0x00);
		fill_rect(left_paddle_x, last_left_paddle_y, paddle_width,
				paddle_height, 0x00);
		fill_rect(right_paddle_x, last_right_paddle_y, paddle_width,
				paddle_height, 0x00);

		// Draw current
		fill_rect(left_paddle_x, left_paddle_y, paddle_width, paddle_height,
				0xFF);
		fill_rect(right_paddle_x, right_paddle_y, paddle_width, paddle_height,
				0xFF);
		fill_rect(ball_x, ball_y, ball_size, ball_size, 0xFF);

		if (ball_y < 49 && (ball_x + ball_size > 160 && ball_x < 340)) {
			sprintf(score_str, "%d-%d", left_score, right_score);
			background_color = "black";
			print_ILI9488(score_str, 180, 0, 2);
			background_color = "red";
			print_ILI9488("X", 305, 0, 2);
		}
		gpio_set_level(SS_display, 1);

		vTaskDelay(50);
	}
}

void pong_gamePvA_task(void *pvParameters) {
	int default_ball_speed_x = 10;   // Ball speed horizontally
	int default_ball_speed_y = 6;
	int paddle_speed = 4;           // Default paddle speed (will be updated)
	int ai_paddle_speed = 4;           // Default paddle speed (will be updated)

	coord_index_char = 1;
	bootApp_pong_B();

	bool break_v = 1;

	printf("PvA \n");

	while (break_v) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= 4; i++) {
			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {

				gpio_set_level(SS_display, 0);
				clean_screen();
				send_command(0x00);
				gpio_set_level(SS_display, 1);

				if (strcmp(history_char[coord_index_char - i].app_name, "close")
						== 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task",
							2048, NULL, 5, &main_menu_Handle);
					vTaskDelete(NULL);
				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Easy") == 0) {
					default_ball_speed_x = 10;
					default_ball_speed_y = 6;
					paddle_speed = 5;
					break_v = 0;
					break;
				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Medium") == 0) {
					default_ball_speed_x = 15;
					default_ball_speed_y = 9;
					paddle_speed = 6;
					ai_paddle_speed = 8;

					break_v = 0;
					break;
				} else {
					// Chameya Mabloula mode
					default_ball_speed_x = 20;
					default_ball_speed_y = 12;
					paddle_speed = 5;  // Faster paddle movement
					ai_paddle_speed = 13;  // Faster paddle movement
					break_v = 0;
					break;
				}
			}
		}
	}

	float ball_speed_x = default_ball_speed_x;
	float ball_speed_y = default_ball_speed_y;
	coord_index_char = 1;

	char score_str[16];
	int player_score = 0;
	int ai_score = 0;

	gpio_set_level(SS_display, 0);
	sprintf(score_str, "%d-%d", player_score, ai_score);
	background_color = "black";
	print_ILI9488(score_str, 180, 0, 2);
	strcpy(history_char[coord_index_char].app_name, "close");
	history_char[coord_index_char].x = 305;
	history_char[coord_index_char].y = 0;
	history_char[coord_index_char].width = 24;
	history_char[coord_index_char].height = 29;
	coord_index_char++;
	background_color = "red";
	print_ILI9488("X", 305, 0, 2);
	gpio_set_level(SS_display, 1);

	int last_ball_x = ball_x;
	int last_ball_y = ball_y;
	int last_left_paddle_y = left_paddle_y;
	int last_right_paddle_y = right_paddle_y;

	srand((unsigned int) esp_timer_get_time());

	while (1) {
		uint16_t x_touch, y_touch;

		last_left_paddle_y = left_paddle_y;
		last_right_paddle_y = right_paddle_y;
		last_ball_x = ball_x;
		last_ball_y = ball_y;

		// Touch input (Player Left Paddle)
		if (calculate_x_y(&x_touch, &y_touch)) {
			if (x_touch < 470) {
				if (y_touch < 160)
					left_paddle_y -= paddle_speed;
				else if (y_touch > 160)
					left_paddle_y += paddle_speed;
			}
		}

		if (x_touch >= 305 && x_touch <= 329 && y_touch <= 29) {

			gpio_set_level(SS_display, 0);
			clean_screen();
			fill_rect(last_ball_x, last_ball_y, ball_size, ball_size, 0x00);
			fill_rect(ball_x, ball_y, ball_size, ball_size, 0x00);
			fill_rect(0, 0, 25, 320, 0x00);
			fill_rect(460, 0, 25, 320, 0x00);
			fill_rect(0, 0, 480, 30, 0x00);

			send_command(0x00);
			gpio_set_level(SS_display, 1);

			xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task", 2048, NULL,
					5, &main_menu_Handle);

			vTaskDelete(NULL);
		}

		// Clamp player paddle
		if (left_paddle_y < 0)
			left_paddle_y = 0;
		if (left_paddle_y > 320 - paddle_height)
			left_paddle_y = 320 - paddle_height;

		// Ball wall collision
		if (ball_y < 0 || ball_y >= 320 - ball_size)
			ball_speed_y = -ball_speed_y;

		// Ball movement
		ball_x += ball_speed_x;
		ball_y += ball_speed_y;

		// Left paddle collision
		if (ball_x <= left_paddle_x + paddle_width
				&& ball_y + ball_size >= left_paddle_y
				&& ball_y <= left_paddle_y + paddle_height) {

			float relative_intersect_y = (left_paddle_y
					+ (rand() % paddle_height)) - (ball_y + ball_size / 2.0f);
			float normalized = clampf(
					relative_intersect_y / (paddle_height / 2.0f), -1.0f, 1.0f);
			float bounce_angle = normalized * (3.14159f / 3);

			float speed = sqrtf(
					ball_speed_x * ball_speed_x + ball_speed_y * ball_speed_y);
			ball_speed_x = (speed * cosf(bounce_angle));
			ball_speed_y = (-speed * sinf(bounce_angle));
			if (ball_speed_x == 0)
				ball_speed_x = default_ball_speed_x;
		}

		// Right paddle collision
		if (ball_x >= right_paddle_x - ball_size
				&& ball_y + ball_size >= right_paddle_y
				&& ball_y <= right_paddle_y + paddle_height) {

			float relative_intersect_y = (right_paddle_y
					+ (rand() % paddle_height)) - (ball_y + ball_size / 2.0f);
			float normalized = clampf(
					relative_intersect_y / (paddle_height / 2.0f), -1.0f, 1.0f);
			float bounce_angle = normalized * (3.14159f / 3);

			float speed = sqrtf(
					ball_speed_x * ball_speed_x + ball_speed_y * ball_speed_y);
			ball_speed_x = (-speed * cosf(bounce_angle));
			ball_speed_y = (-speed * sinf(bounce_angle));
			if (ball_speed_x == 0)
				ball_speed_x = -default_ball_speed_x;
		}

		// Scoring
		if (ball_x >= 480) {
			player_score++;
			ball_x = 240;
			ball_y = 160;
			ball_speed_x = -1 * default_ball_speed_x;
			ball_speed_y = 0;
			sprintf(score_str, "%d-%d", player_score, ai_score);
			gpio_set_level(SS_display, 0);
			background_color = "black";
			print_ILI9488(score_str, 180, 0, 2);
			gpio_set_level(SS_display, 1);
		}
		if (ball_x <= 0) {
			ai_score++;
			ball_x = 240;
			ball_y = 160;
			ball_speed_x = default_ball_speed_x;
			ball_speed_y = 0;

			gpio_set_level(SS_display, 0);
			sprintf(score_str, "%d-%d", player_score, ai_score);
			background_color = "black";
			print_ILI9488(score_str, 180, 0, 2);
			gpio_set_level(SS_display, 1);
		}

		// AI Movement (Right Paddle with randomness)
		int right_offset = (rand() % paddle_height) - paddle_height / 2;
		if (right_paddle_y + paddle_height / 2 + right_offset < ball_y)
			right_paddle_y += ai_paddle_speed;
		else if (right_paddle_y + paddle_height / 2 + right_offset > ball_y)
			right_paddle_y -= ai_paddle_speed;

		if (right_paddle_y < 0)
			right_paddle_y = 0;
		if (right_paddle_y > 320 - paddle_height)
			right_paddle_y = 320 - paddle_height;

		// Render updates
		gpio_set_level(SS_display, 0);
		fill_rect(last_ball_x, last_ball_y, ball_size, ball_size, 0x00);
		fill_rect(left_paddle_x, last_left_paddle_y, paddle_width,
				paddle_height, 0x00);
		fill_rect(right_paddle_x, last_right_paddle_y, paddle_width,
				paddle_height, 0x00);
		fill_rect(left_paddle_x, left_paddle_y, paddle_width, paddle_height,
				0xFF);
		fill_rect(right_paddle_x, right_paddle_y, paddle_width, paddle_height,
				0xFF);
		fill_rect(ball_x, ball_y, ball_size, ball_size, 0xFF);

		if (ball_y < 49 && (ball_x + ball_size > 160 && ball_x < 340)) {
			sprintf(score_str, "%d-%d", player_score, ai_score);
			background_color = "black";
			print_ILI9488(score_str, 180, 0, 2);
			background_color = "red";
			print_ILI9488("X", 305, 0, 2);
		}
		gpio_set_level(SS_display, 1);

		vTaskDelay(50);
	}
}

void pong_gameAvA_task(void *pvParameters) {
	int default_ball_speed_x = 10;   // Ball speed horizontally
	int default_ball_speed_y = 6;
	int paddle_speed = 4;           // Default paddle speed (will be updated)

	coord_index_char = 1;

	bootApp_pong_B();

	bool break_v = 1;

	while (break_v) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= 4; i++) {
			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {

				gpio_set_level(SS_display, 0);
				clean_screen();
				send_command(0x00);
				gpio_set_level(SS_display, 1);

				if (strcmp(history_char[coord_index_char - i].app_name, "close")
						== 0) {
					xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task",
							2048, NULL, 5, &main_menu_Handle);
					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Easy") == 0) {
					default_ball_speed_x = 10;
					default_ball_speed_y = 6;
					paddle_speed = 4;
					break_v = 0;
					break;

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Medium") == 0) {
					default_ball_speed_x = 15;
					default_ball_speed_y = 9;
					paddle_speed = 6;
					break_v = 0;
					break;

				} else {
					// Chameya Mabloula mode
					default_ball_speed_x = 20;
					default_ball_speed_y = 12;
					paddle_speed = 13;  // Faster paddle movement
					break_v = 0;
					break;
				}
			}
		}
	}

	float ball_speed_x = default_ball_speed_x;
	float ball_speed_y = default_ball_speed_y;
	coord_index_char = 1;

	char score_str[16];
	int left_ai_score = 0;
	int right_ai_score = 0;

	gpio_set_level(SS_display, 0);
	sprintf(score_str, "%d-%d", left_ai_score, right_ai_score);
	background_color = "black";
	print_ILI9488(score_str, 180, 0, 2);
	strcpy(history_char[coord_index_char].app_name, "close");
	history_char[coord_index_char].x = 305;
	history_char[coord_index_char].y = 0;
	history_char[coord_index_char].width = 24;
	history_char[coord_index_char].height = 29;
	coord_index_char++;
	background_color = "red";
	print_ILI9488("X", 305, 0, 2);
	gpio_set_level(SS_display, 1);

	int last_ball_x = ball_x;
	int last_ball_y = ball_y;
	int last_left_paddle_y = left_paddle_y;
	int last_right_paddle_y = right_paddle_y;

	srand((unsigned int) esp_timer_get_time());

	uint16_t x_touch, y_touch;

	while (1) {

		// Touch input (Player Left Paddle)
		if (calculate_x_y(&x_touch, &y_touch)) {
			if (x_touch < 470) {
				if (y_touch < 160)
					left_paddle_y -= paddle_speed;
				else if (y_touch > 160)
					left_paddle_y += paddle_speed;
			} else {
				continue;
			}
		}

		if (x_touch >= 305 && x_touch <= 329 && y_touch <= 29) {

			gpio_set_level(SS_display, 0);
			clean_screen();
			fill_rect(last_ball_x, last_ball_y, ball_size, ball_size, 0x00);
			fill_rect(ball_x, ball_y, ball_size, ball_size, 0x00);
			fill_rect(0, 0, 25, 320, 0x00);
			fill_rect(460, 0, 25, 320, 0x00);
			fill_rect(0, 0, 480, 30, 0x00);

			send_command(0x00);
			gpio_set_level(SS_display, 1);

			xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task", 2048, NULL,
					5, &main_menu_Handle);

			vTaskDelete(NULL);
		}

		last_left_paddle_y = left_paddle_y;
		last_right_paddle_y = right_paddle_y;
		last_ball_x = ball_x;
		last_ball_y = ball_y;

		int left_offset = (rand() % paddle_height) - paddle_height / 2;
		int right_offset = (rand() % paddle_height) - paddle_height / 2;

		// AI Movement for Left Paddle
		if (left_paddle_y + paddle_height / 2 + left_offset < ball_y)
			left_paddle_y += paddle_speed;
		else if (left_paddle_y + paddle_height / 2 + left_offset > ball_y)
			left_paddle_y -= paddle_speed;

		if (left_paddle_y < 0)
			left_paddle_y = 0;
		if (left_paddle_y > 320 - paddle_height)
			left_paddle_y = 320 - paddle_height;

		// AI Movement for Right Paddle
		if (right_paddle_y + paddle_height / 2 + right_offset < ball_y)
			right_paddle_y += paddle_speed;
		else if (right_paddle_y + paddle_height / 2 + right_offset > ball_y)
			right_paddle_y -= paddle_speed;

		if (right_paddle_y < 0)
			right_paddle_y = 0;
		if (right_paddle_y > 320 - paddle_height)
			right_paddle_y = 320 - paddle_height;

		// Wall collision
		if (ball_y < 0 || ball_y >= 320 - ball_size)
			ball_speed_y = -ball_speed_y;

		// Ball movement
		ball_x += ball_speed_x;
		ball_y += ball_speed_y;

		// Left paddle collision
		if (ball_x <= left_paddle_x + paddle_width
				&& ball_y + ball_size >= left_paddle_y
				&& ball_y <= left_paddle_y + paddle_height) {

			float relative_intersect_y = (left_paddle_y
					+ (rand() % paddle_height)) - (ball_y + ball_size / 2.0f);
			float normalized = clampf(
					relative_intersect_y / (paddle_height / 2.0f), -1.0f, 1.0f);
			float bounce_angle = normalized * (3.14159f / 3);

			float speed = sqrtf(
					ball_speed_x * ball_speed_x + ball_speed_y * ball_speed_y);
			ball_speed_x = (speed * cosf(bounce_angle));
			ball_speed_y = (-speed * sinf(bounce_angle));
			if (ball_speed_x == 0)
				ball_speed_x = default_ball_speed_x;
		}

		// Right paddle collision
		if (ball_x >= right_paddle_x - ball_size
				&& ball_y + ball_size >= right_paddle_y
				&& ball_y <= right_paddle_y + paddle_height) {

			float relative_intersect_y = (right_paddle_y
					+ (rand() % paddle_height)) - (ball_y + ball_size / 2.0f);
			float normalized = clampf(
					relative_intersect_y / (paddle_height / 2.0f), -1.0f, 1.0f);
			float bounce_angle = normalized * (3.14159f / 3);

			float speed = sqrtf(
					ball_speed_x * ball_speed_x + ball_speed_y * ball_speed_y);
			ball_speed_x = (-speed * cosf(bounce_angle));
			ball_speed_y = (-speed * sinf(bounce_angle));
			if (ball_speed_x == 0)
				ball_speed_x = -default_ball_speed_x;
		}

		// Scoring
		if (ball_x >= 480) {
			left_ai_score++;
			ball_x = 240;
			ball_y = 160;
			ball_speed_x = default_ball_speed_x;
			ball_speed_y = 0;
			sprintf(score_str, "%d-%d", left_ai_score, right_ai_score);
			gpio_set_level(SS_display, 0);
			background_color = "black";
			print_ILI9488(score_str, 180, 0, 2);
			gpio_set_level(SS_display, 1);
		}
		if (ball_x <= 0) {
			right_ai_score++;
			ball_x = 240;
			ball_y = 160;
			ball_speed_x = -default_ball_speed_x;
			ball_speed_y = 0;

			sprintf(score_str, "%d-%d", left_ai_score, right_ai_score);
			gpio_set_level(SS_display, 0);
			background_color = "black";
			print_ILI9488(score_str, 180, 0, 2);
			gpio_set_level(SS_display, 1);
		}

		// Render updates
		gpio_set_level(SS_display, 0);
		fill_rect(last_ball_x, last_ball_y, ball_size, ball_size, 0x00);
		fill_rect(left_paddle_x, last_left_paddle_y, paddle_width,
				paddle_height, 0x00);
		fill_rect(right_paddle_x, last_right_paddle_y, paddle_width,
				paddle_height, 0x00);
		fill_rect(left_paddle_x, left_paddle_y, paddle_width, paddle_height,
				0xFF);
		fill_rect(right_paddle_x, right_paddle_y, paddle_width, paddle_height,
				0xFF);
		fill_rect(ball_x, ball_y, ball_size, ball_size, 0xFF);

		if (ball_y < 49 && (ball_x + ball_size > 160 && ball_x < 340)) {
			sprintf(score_str, "%d-%d", left_ai_score, right_ai_score);
			background_color = "black";
			print_ILI9488(score_str, 180, 0, 2);
			background_color = "red";
			print_ILI9488("X", 305, 0, 2);
		}

		gpio_set_level(SS_display, 1);

		vTaskDelay(50);
	}
}

void pong_gamePage1_task(void *pvParameters) {
	coord_index_char = 1;

	bootApp_pong();

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= 4; i++) {
			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {
				gpio_set_level(SS_display, 0);

				if (strcmp(history_char[coord_index_char - i].app_name, "close")
						== 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();
					send_command(0x00);

					gpio_set_level(SS_display, 1);

					xTaskCreate(main_menu_task, "main_menu_task", 2048,
					NULL, 5, &main_menu_Handle);

					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"AI vs AI") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();
					send_command(0x00);

					gpio_set_level(SS_display, 1);

					xTaskCreate(pong_gameAvA_task, "pong_gameAvA_task", 2048,
					NULL, 5, &main_menu_Handle);

					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Player vs AI") == 0) {

					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(pong_gamePvA_task, "pong_gamePvA_task", 2048,
					NULL, 5, &other_task_handel);

					vTaskDelete(NULL);

				} else {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(pong_gamePvP_task, "pong_gamePvP_task", 2048,
					NULL, 5, &other_task_handel);
					vTaskDelete(NULL);

				}

			}
			send_command(0x00);
			gpio_set_level(SS_display, 1);
		}
	}
}

void keyboard_task(void *pvParameters) {

	TaskParams *data = (TaskParams*) pvParameters;

	uint16_t x1 = data->x;
	uint16_t y1 = data->y;
	char *previous_task = data->previous_task;
	char *current_task = data->current_task;

	char case_type = 'l';

	draw_keyborad(case_type);

	while (1) {

		vTaskDelay(100);

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		check_key_press(x, y, &x1, &y1, &case_type, previous_task, current_task,
				data);

		send_command(0x00);
		gpio_set_level(SS_display, 1);
	}

}

void notebook_editFilesPage2_task(void *pvParameters) {

	char *filename = (char*) pvParameters;

	make_button("Edit", 100, 40, 100);
	make_button("Delete file", 100, 200, 100);

	make_X_button();

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i < 4; i++) {

			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {

				if (strcmp(history_char[coord_index_char - i].app_name,
						"Delete file") == 0) {

					char full_path_l[50];

					snprintf(full_path_l, sizeof(full_path_l),
							"/spiffs/notebook/%s\n", filename);

					delete_file(full_path_l);

					gpio_set_level(SS_display, 0);

					clean_screen();

					while (coord_index_char > 0) {
						clean_last_char();
					}

					gpio_set_level(SS_display, 1);

					free(filename);

					xTaskCreate(notebook_editFilesPage1_task,
							"notebook_editFilesPage1_task", 2048,
							NULL, 5, &other_task_handel);

					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"close") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					while (coord_index_char > 0) {
						clean_last_char();
					}

					gpio_set_level(SS_display, 1);

					free(filename);

					xTaskCreate(notebook_editFilesPage1_task,
							"notebook_editFilesPage1_task", 2048,
							NULL, 5, &other_task_handel);

					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Edit") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					while (coord_index_char > 0) {
						clean_last_char();
					}

					snprintf(full_path, sizeof(full_path),
							"/spiffs/notebook/%s\n", filename);

					print_ILI9488(filename, 100, 0, 2);

					char *contents_local = read_file_contents(full_path);
					char *original_ptr = contents_local; // Save original pointer

					uint16_t x1 = 0, y1 = 35;

					keyboard_buffer_i = 0;
					coord_index_char = 1;

					while (*contents_local) {
						char c = *contents_local;

						background_color = "red";
						print_char_ILI9488(c, &x1, &y1, 2);
						keyboard_buffer[keyboard_buffer_i++] = c;

						send_command(0x00);
						contents_local++;
					}

					gpio_set_level(SS_display, 1);

					TaskParams *params = malloc(sizeof(TaskParams));
					params->x = x1;
					params->y = y1;

					strcpy(params->previous_task,
							"notebook_editFilesPage2_task");
					strcpy(params->current_task, "keyboard_to_edit");

					paragraph_number = 1;

					xTaskCreate(keyboard_task, "keyboard_task_to_edit", 2048,
							(void*) params, 5, &other_task_handel);

					free(filename);

					free(original_ptr);  // Free the memory properly

					vTaskDelete(NULL);

				}

			}
		}

	}
}

void notebook_editFilesPage1_task(void *pvParameters) {

	gpio_set_level(SS_display, 0);

	background_color = "red";
	print_ILI9488("files", 100, 5, 2);

	gpio_set_level(SS_display, 1);

	char *content = read_file_contents("/spiffs/notebook/filenames.txt");

	uint16_t width = 70;
	uint16_t height = 70;
	uint16_t x = 5;
	uint16_t y = 35;
	uint8_t file_count = 0;
	uint8_t x_level = 0;

	char *filenames[MAX_FILES]; // Array of pointers

	coord_index_char = 1;

	if (content) {
		printf("File content:\n");

		char *line = strtok(content, "\n");

		while (line != NULL && file_count < MAX_FILES) {
			printf("File: %s\n", line);

			// Store a copy of the token
			filenames[file_count] = strdup(line); // allocates and copies the string

			int filename_length = strlen(filenames[file_count]); // Length of the filename
			width = 24 * filename_length + 24;  // Multiply the length by 24

			if (x + width > 420) {
				y += height + 5;
				x = 5;
			}

			make_button(line, height, x, y);

			if (x + width <= 420) {
				x += width + 5;
			}

			file_count++;
			line = strtok(NULL, "\n");
		}

	}

	free(content);

	make_X_button();

	file_count++;

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= file_count + 1; i++) {
			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {
				if (strcmp(history_char[coord_index_char - i].app_name, "close")
						!= 0 && (x_level != 1)) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					while (coord_index_char > 0) {
						clean_last_char();
					}

					print_ILI9488(filenames[file_count - i], 100, 5, 2);

					gpio_set_level(SS_display, 1);

					char *filename_copy = strdup(filenames[file_count - i]);

					for (uint8_t i = 0; i <= file_count; i++) {
						free(filenames[i]);
					}

					xTaskCreate(notebook_editFilesPage2_task,
							"notebook_editFilesPage2_task", 2048,
							(void*) filename_copy, // <- pass it here
							5, &main_menu_Handle);

					x_level = 1;

					vTaskDelete(NULL);

				} else if (x_level == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					while (coord_index_char > 0) {
						clean_last_char();
					}

					gpio_set_level(SS_display, 1);

					free(content);  // Don't forget to free the memory!

					for (uint8_t i = 0; i <= file_count; i++) {
						free(filenames[i]);
					}

					xTaskCreate(note_book_app_page1, "note_book_app_page1",
							2048,
							NULL, 5, &other_task_handel);

					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"close") == 0) {
					uint16_t x = 5;
					uint16_t y = 35;

					gpio_set_level(SS_display, 0);

					clean_screen();

					while (coord_index_char > 0) {
						clean_last_char();
					}

					background_color = "red";
					print_ILI9488("files", 100, 5, 2);

					x_level = 0;

					coord_index_char = 1;

					for (uint8_t j = 0; j < file_count - 1; j++) {

						int filename_length = strlen(filenames[file_count]); // Length of the filename
						width = 24 * filename_length + 24; // Multiply the length by 24

						set_resolution_pos(x, y, width, height, 0);

						send_command(0x2C);

						for (uint64_t i = 0; i < width * height / 2; i++) {
							send_ILI9488_data(0x24);
						}

						print_ILI9488(filenames[j], x + 15, y + 15, 2);

						if (x + width > 420) {
							y += height + 5;
							x = 5;
						} else {
							x += width + 5;
						}

						coord_index_char++;

					}

					make_X_button();

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					break;

				}
			}
		}
	}
}

void notebook_readfiles_task(void *pvParameters) {

	gpio_set_level(SS_display, 0);

	background_color = "red";
	print_ILI9488("files", 100, 5, 2);

	gpio_set_level(SS_display, 1);

	char *content = read_file_contents("/spiffs/notebook/filenames.txt");

	uint16_t width = 70;
	uint16_t height = 70;
	uint16_t x = 5;
	uint16_t y = 35;
	uint8_t file_count = 0;

	bool x_level = 0;

	char *filenames[MAX_FILES]; // Array of pointers

	coord_index_char = 1;

	if (content) {
		printf("File content:\n");

		char *line = strtok(content, "\n");

		while (line != NULL && file_count < MAX_FILES) {
			printf("File: %s\n", line);

			// Store a copy of the token
			filenames[file_count] = strdup(line); // allocates and copies the string

			int filename_length = strlen(filenames[file_count]); // Length of the filename
			width = 24 * filename_length + 24;  // Multiply the length by 24

			if (x + width > 420) {
				y += height + 5;
				x = 5;
			}

			make_button(line, height, x, y);

			if (x + width <= 420) {
				x += width + 5;
			}

			file_count++;
			line = strtok(NULL, "\n");
		}

	}

	free(content);

	make_X_button();

	file_count++;

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= file_count + 1; i++) {
			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {
				if (strcmp(history_char[coord_index_char - i].app_name, "close")
						!= 0 && (x_level != 1)) {
					gpio_set_level(SS_display, 0);

					char full_path_l[50];

					clean_screen();

					background_color = "red";
					print_ILI9488("X", 456, 0, 2);
					background_color = "red";

					snprintf(full_path_l, sizeof(full_path_l),
							"/spiffs/notebook/%s\n", filenames[file_count - i]);

					print_ILI9488(filenames[file_count - i], 100, 0, 2);

					char *contents_local = read_file_contents(full_path_l);

					print_ILI9488(contents_local, 0, 35, 2);

					free(contents_local);  // Don't forget to free the memory!

					x_level = 1;

					send_command(0x00);

					gpio_set_level(SS_display, 1);

					break;

				} else if (x_level == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					while (coord_index_char > 0) {
						clean_last_char();
					}

					gpio_set_level(SS_display, 1);

					free(content);  // Don't forget to free the memory!

					xTaskCreate(note_book_app_page1, "note_book_app_page1",
							2048,
							NULL, 5, &other_task_handel);

					for (uint8_t i = 0; i <= file_count; i++) {
						free(filenames[i]);
					}

					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"close") == 0) {
					uint16_t x = 5;
					uint16_t y = 35;

					gpio_set_level(SS_display, 0);

					clean_screen();

					while (coord_index_char > 0) {
						clean_last_char();
					}

					background_color = "red";
					print_ILI9488("files", 100, 5, 2);

					x_level = 0;

					coord_index_char = 1;

					for (uint8_t j = 0; j < file_count - 1; j++) {

						int filename_length = strlen(filenames[j]); // Length of the filename

						width = 24 * filename_length + 24; // Multiply the length by 24

						if (x + width > 420) {
							y += height + 5;
							x = 5;
						}

						set_resolution_pos(x, y, width, height, 0);

						send_command(0x2C);

						for (uint64_t i = 0; i < width * height / 2; i++) {
							send_ILI9488_data(0x24);
						}

						send_command(0x00);

						print_ILI9488(filenames[j], x + 15, y + 15, 2);

						if (x + width <= 420) {
							x += width + 5;
						}

						coord_index_char++;

					}

					make_X_button();

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					break;

				}

			}
		}
	}
}

void main_menu_task(void *pvParameters) {

	draw_main_menu_icons();

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 0; i <= 2; i++) {

			if ((x >= history[coord_index - i].x
					&& x
							<= (history[coord_index - i].x
									+ history[coord_index - i].width))
					&& (y >= history[coord_index - i].y
							&& y
									<= (history[coord_index - i].y
											+ history[coord_index - i].height))) {
				if (strcmp(history[coord_index - i].app_name, "notebook")
						== 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(note_book_app_page1, "note_book_app_page1",
							2048,
							NULL, 5, &main_menu_Handle);

					vTaskDelete(NULL);

				} else if (strcmp(history[coord_index - i].app_name, "pong")
						== 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task",
							2048,
							NULL, 5, &main_menu_Handle);

					vTaskDelete(NULL);

				}
			}

		}
	}
}

void note_book_app_page1(void *pvParameters) {

	coord_index_char = 1;

	bootApp_noteBook();

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= 4; i++) {
			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {
				gpio_set_level(SS_display, 0);

				if (strcmp(history_char[coord_index_char - i].app_name, "close")
						== 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();

					xTaskCreate(main_menu_task, "main_menu_task", 2048,
					NULL, 5, &main_menu_Handle);

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"New file") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					background_color = "red";
					print_ILI9488("New file name", 100, 5, 2);

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					TaskParams *params = malloc(sizeof(TaskParams));
					params->x = 0;
					params->y = 35;
					strcpy(params->previous_task, "note_book_app_page1");
					strcpy(params->current_task, "keyboard_New_file");

					coord_index_char = 1;

					xTaskCreate(keyboard_task, "keyboard_task", 2048,
							(void*) params, 5, &main_menu_Handle);

					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Read files") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					xTaskCreate(notebook_readfiles_task,
							"notebook_readfiles_task", 2048, NULL, 5,
							&other_task_handel);

					vTaskDelete(NULL);

				} else {

					gpio_set_level(SS_display, 0);

					clean_screen();

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					xTaskCreate(notebook_editFilesPage1_task,
							"notebook_editFiles_task", 2048,
							NULL, 5, &other_task_handel);

					vTaskDelete(NULL);

				}

			}
			send_command(0x00);
			gpio_set_level(SS_display, 1);

		}
	}

}

