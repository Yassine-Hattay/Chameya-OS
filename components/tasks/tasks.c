/*
 * tasks.c
 *
 *  Created on: 15 Apr 2025
 *      Author: hatta
 */

#include "tasks.h"

#define MAX_FILES 100
#define MAX_FILENAME_LEN 64
// Touch regions for UP and DOWN
#define BUTTON_UP_X_START 0
#define BUTTON_UP_X_END 240
#define BUTTON_UP_Y_START 0
#define BUTTON_UP_Y_END 100

#define BUTTON_DOWN_X_START 0
#define BUTTON_DOWN_X_END 240
#define BUTTON_DOWN_Y_START 220
#define BUTTON_DOWN_Y_END 320

#define ATTACK_RANGE 20
#define MAX_X 480  // Max screen width (example, adjust as needed)
#define MAX_Y 320  // Max screen height (example, adjust as needed)
#define MIN_X 0    // Min screen width
#define MIN_Y 0

bool Read_write_bit_i2c;

typedef struct {
	const char *protocol;
	uint8_t val1;
	uint8_t val2;
	uint8_t val3;
} Confirm_struct;

goblin_group_t *global_group = NULL;

float scaling_factor = 1.013; // Adjust this for the desired speed of growth
float cell_size = 1.875;
float cell_size1 = 1.25;

TaskHandle_t main_menu_Handle = NULL;
TaskHandle_t other_task_handel = NULL;

char keyboard_buffer[MAX_COORDS_CHAR];
uint8_t keyboard_buffer_i = 0;

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

static bool death_bool = true;

goblin_torch goblins[NUM_GOBLINS];

map myMap;

TaskHandle_t goblinHandles[NUM_GOBLINS]; // Assume goblin1Handle to goblin6Handle assigned here

static int current_goblin_index = 0;  // Global variable

// goblin_torch goblins[NUM_GOBLINS];
const char *goblinNames[NUM_GOBLINS] = { "G1", "G2", "G3", "G4", "G5", "G6",
		"G7", "G8", "G9", "G10" };

static int goblin_nb = 0;

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

void confirm(void *pvParameters) {
	Confirm_struct *data = (Confirm_struct*) pvParameters;

	coord_index_char = 1;

	background_color = "black";

	char buffer[16];  // Safe for "TX:255" + '\0'
	if (strcmp(data->protocol, "uart") == 0) {
		gpio_set_level(SS_display, 0);

		print_ILI9488("confirm ?", 100, 0, 2);
		confirm_boot();
		sprintf(buffer, "TX:%d", data->val1);
		print_ILI9488(buffer, 10, 50, 2);
		sprintf(buffer, "RX:%d", data->val2);
		print_ILI9488(buffer, 200, 50, 2);
		send_command(0x00);
		gpio_set_level(SS_display, 1);
	} else if (strcmp(data->protocol, "I2C") == 0) {

		if (!data->val3) {

			I2C_SDA = data->val1;
			I2C_SCL = data->val2;
			Read_write_bit_i2c = data->val3;

			gpio_config_t io_conf_SCL_output;
			io_conf_SCL_output.intr_type = GPIO_INTR_DISABLE; // Disable interrupt
			io_conf_SCL_output.mode = GPIO_MODE_OUTPUT; // Set as output mode
			io_conf_SCL_output.pin_bit_mask = (1ULL << I2C_SCL); // Set both SDA and SCL
			io_conf_SCL_output.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
			io_conf_SCL_output.pull_up_en = GPIO_PULLUP_DISABLE; // Disable pull-up
			gpio_config(&io_conf_SCL_output);

			gpio_config_t io_conf_SDA_output = { .pin_bit_mask = (1ULL
					<< I2C_SDA), .mode = GPIO_MODE_OUTPUT, // MOSI, SCK, and SS as input
					.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
							GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
					};

			gpio_config(&io_conf_SDA_output);

			paragraph_number = 1;

			gpio_set_level(SS_display, 0);
			clean_screen();
			print_ILI9488("Slave address", 80, 0, 2);
			gpio_set_level(SS_display, 1);

			TaskParams *params = malloc(sizeof(TaskParams));
			strcpy(params->current_task, "GPIO_C_I2C_Address");
			params->x = 0;
			params->y = 35;
			paragraph_number = 1;
			coord_index_char = 1;

			xTaskCreate(keyboard_task, "GPIO_C_I2C_Address", 2048,
					(void*) params, 5, &other_task_handel);
			vTaskDelete(NULL);
		} else {

			Read_write_bit_i2c = data->val3;
			I2C_SDA = data->val1;
			I2C_SCL = data->val2;

			gpio_config_t io_conf_SCL_output;
			io_conf_SCL_output.intr_type = GPIO_INTR_DISABLE; // Disable interrupt
			io_conf_SCL_output.mode = GPIO_MODE_OUTPUT; // Set as output mode
			io_conf_SCL_output.pin_bit_mask = (1ULL << I2C_SCL); // Set both SDA and SCL
			io_conf_SCL_output.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
			io_conf_SCL_output.pull_up_en = GPIO_PULLUP_DISABLE; // Disable pull-up
			gpio_config(&io_conf_SCL_output);

			gpio_config_t io_conf_SDA_output = { .pin_bit_mask = (1ULL
					<< I2C_SDA), .mode = GPIO_MODE_OUTPUT, // MOSI, SCK, and SS as input
					.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
							GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
					};

			gpio_config(&io_conf_SDA_output);

			paragraph_number = 1;

			gpio_set_level(SS_display, 0);
			clean_screen();
			print_ILI9488("Slave address", 80, 0, 2);
			gpio_set_level(SS_display, 1);

			TaskParams *params = malloc(sizeof(TaskParams));
			strcpy(params->current_task, "GPIO_C_I2C_Address");
			params->x = 0;
			params->y = 35;
			paragraph_number = 1;
			coord_index_char = 1;

			xTaskCreate(keyboard_task, "GPIO_C_I2C_Address", 2048,
					(void*) params, 5, &other_task_handel);
			vTaskDelete(NULL);

		}
	} else {
		gpio_set_level(SS_display, 0);

		print_ILI9488("confirm ?", 100, 0, 2);
		confirm_boot();

		sprintf(buffer, "CS:%d", data->val1);
		printf("CS: %d\n", data->val1);

		print_ILI9488(buffer, 10, 50, 2);
		sprintf(buffer, "CLK:%d", data->val2);

		print_ILI9488(buffer, 160, 50, 2);
		printf("CLK: %d\n", data->val2);

		sprintf(buffer, "MOSI:%d", data->val3);
		printf("MOSI: %d\n", data->val3);
		print_ILI9488(buffer, 340, 50, 2);
		send_command(0x00);
		gpio_set_level(SS_display, 1);
	}

	uint8_t num_buttons = 2;

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= num_buttons; i++) {

			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {

				if (strcmp(history_char[coord_index_char - i].app_name,
						"Transmit") == 0) {
					if (strcmp(data->protocol, "uart") == 0) {

						gpio_set_level(SS_display, 0);
						clean_screen();
						print_ILI9488("baud rate : 9600", 80, 0, 2);
						gpio_set_level(SS_display, 1);

						gpio_set_direction(data->val1, GPIO_MODE_OUTPUT);
						gpio_set_level(data->val1, 1);  // Idle state is high
						TX_PIN = data->val1;

						TaskParams *params = malloc(sizeof(TaskParams));
						strcpy(params->current_task, "GPIO_C_UART_Transmit");
						params->x = 0;
						params->y = 35;
						paragraph_number = 1;
						coord_index_char = 1;

						xTaskCreate(keyboard_task, "GPIO_C_UART_Transmit", 2048,
								(void*) params, 5, &other_task_handel);
						vTaskDelete(NULL);

					} else {

						gpio_set_level(SS_display, 0);
						clean_screen();
						print_ILI9488("send data", 80, 0, 2);
						gpio_set_level(SS_display, 1);

						BSS = data->val1;
						BSCK = data->val2;
						BMOSI = data->val3;

						TaskParams *params = malloc(sizeof(TaskParams));
						strcpy(params->current_task, "GPIO_C_SPI_Transmit");
						params->x = 0;
						params->y = 35;
						paragraph_number = 1;
						coord_index_char = 1;

						gpio_config_t io_conf = { .pin_bit_mask = (1ULL << BMOSI)
								| (1ULL << BSCK) | (1ULL << BSS), .mode =
								GPIO_MODE_OUTPUT, .pull_up_en =
								GPIO_PULLUP_DISABLE, .pull_down_en =
								GPIO_PULLDOWN_DISABLE, .intr_type =
								GPIO_INTR_DISABLE };
						gpio_config(&io_conf);


						gpio_set_level(BMOSI, 0);
						gpio_set_level(BSS, 1);

						xTaskCreate(keyboard_task, "GPIO_C_SPI_Transmit", 2048,
								(void*) params, 5, &other_task_handel);
						vTaskDelete(NULL);

					}

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"close") == 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();

					gpio_set_level(SS_display, 1);

					if (strcmp(data->protocol, "uart") == 0) {
						xTaskCreate(GPIO_C_UART_page_0, "GPIO_C_UART_page_0",
								2048,
								NULL, 5, &main_menu_Handle);

						vTaskDelete(NULL);

					}
				}
			}
		}
	}
}

void GPIO_C_I2C_page_0(void *pvParameters) {
	char buffer[16];

	bool pressed = false;

	coord_index_char = 1;

	Confirm_struct *params = (Confirm_struct*) malloc(sizeof(TaskParams));

	GPIO_pins_boot();

	gpio_set_level(SS_display, 0);
	print_ILI9488("Select SDA pin", 100, 0, 2);
	send_command(0x00);
	gpio_set_level(SS_display, 1);

	uint8_t num_buttons = 4;

	uint8_t SDA_pin = 0;

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= num_buttons; i++) {

			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {

				if (strcmp(history_char[coord_index_char - i].app_name,
						"GPIO 0") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					if (pressed) {

						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);

						background_color = "red";

						int height = 55;

						make_button("Write", height, 50, 100, "red");

						make_button("Read", height, 240, 100, "green");

						gpio_set_level(SS_display, 0);

						params->protocol = "I2C";
						params->val1 = SDA_pin;
						params->val2 = 0;

						sprintf(buffer, "SDA:%d", params->val1);
						print_ILI9488(buffer, 10, 50, 2);
						sprintf(buffer, "SCL:%d", params->val2);
						print_ILI9488(buffer, 200, 50, 2);

						send_command(0x00);

						num_buttons = 3;

					} else {

						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);

						print_ILI9488("Select SCL pin", 100, 0, 2);

						background_color = "red";

						int height = 55;

						make_button("GPIO 2", height, 240, 60, "red");

						make_button("GPIO 12", height, 120, 150, "red");

						send_command(0x00);

						pressed = true;
						SDA_pin = 0;
						num_buttons--;
					}
					gpio_set_level(SS_display, 1);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"GPIO 2") == 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();

					if (pressed) {

						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);

						background_color = "red";

						int height = 55;

						make_button("Write", height, 50, 100, "red");

						make_button("Read", height, 240, 100, "green");
						gpio_set_level(SS_display, 0);

						params->protocol = "I2C";
						params->val1 = SDA_pin;
						params->val2 = 2;

						sprintf(buffer, "SDA:%d", params->val1);
						print_ILI9488(buffer, 10, 50, 2);
						sprintf(buffer, "SCL:%d", params->val2);
						print_ILI9488(buffer, 200, 50, 2);

						send_command(0x00);

						num_buttons = 3;

					} else {
						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						print_ILI9488("X", 456, 0, 2);

						print_ILI9488("Select SCL pin", 100, 0, 2);

						int height = 55;
						make_button("GPIO 0", height, 50, 60, "red");

						make_button("GPIO 12", height, 120, 150, "red");

						send_command(0x00);

						num_buttons--;
						pressed = true;
						SDA_pin = 2;
					}
					gpio_set_level(SS_display, 1);

				}

				else if (strcmp(history_char[coord_index_char - i].app_name,
						"GPIO 12") == 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();

					if (pressed) {
						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);

						background_color = "red";

						int height = 55;

						make_button("Write", height, 50, 100, "red");

						make_button("Read", height, 240, 100, "green");
						gpio_set_level(SS_display, 0);

						params->protocol = "I2C";
						params->val1 = SDA_pin;
						params->val2 = 12;

						sprintf(buffer, "SDA:%d", params->val1);
						print_ILI9488(buffer, 10, 50, 2);
						sprintf(buffer, "SCL:%d", params->val2);
						print_ILI9488(buffer, 200, 50, 2);

						send_command(0x00);

						num_buttons = 3;

					} else {
						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);

						print_ILI9488("Select SCL pin", 100, 0, 2);

						background_color = "red";

						int height = 55;

						make_button("GPIO 0", height, 50, 60, "red");

						make_button("GPIO 2", height, 240, 60, "red");

						send_command(0x00);

						num_buttons--;
						pressed = true;
						SDA_pin = 12;
					}

					gpio_set_level(SS_display, 1);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"close") == 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(GPIO_C_page_1, "GPIO_C_page_1", 2048,
					NULL, 5, &main_menu_Handle);
					vTaskDelete(NULL);
				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Write") == 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					params->val3 = 0;

					xTaskCreate(confirm, "confirm", 2048, (void*) params, 5,
							&main_menu_Handle);
					vTaskDelete(NULL);
				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Read") == 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					params->val3 = 1;

					xTaskCreate(confirm, "confirm", 2048, (void*) params, 5,
							&main_menu_Handle);
					vTaskDelete(NULL);
				}
			}
		}
	}
}

void GPIO_C_SPI_page_0(void *pvParameters) {
	bool pressed = false;

	coord_index_char = 1;

	GPIO_pins_boot();

	gpio_set_level(SS_display, 0);
	print_ILI9488("Select CS pin", 100, 0, 2);
	send_command(0x00);
	gpio_set_level(SS_display, 1);

	uint8_t num_buttons = 4;

	uint8_t CS_pin = 0;

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= num_buttons; i++) {

			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {

				if (strcmp(history_char[coord_index_char - i].app_name,
						"GPIO 0") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					if (pressed) {

						Confirm_struct *params = (Confirm_struct*) malloc(
								sizeof(TaskParams));
						params->protocol = "SPI";
						params->val1 = CS_pin;
						params->val2 = 0;
						if (CS_pin == 2)
							params->val3 = 12;
						else
							params->val3 = 2;

						xTaskCreate(confirm, "confirm", 2048, (void*) params, 5,
								&main_menu_Handle);

						vTaskDelete(NULL);

					} else {

						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);

						print_ILI9488("Select RX pin", 100, 0, 2);

						background_color = "red";

						int height = 55;

						make_button("GPIO 2", height, 240, 60, "red");

						make_button("GPIO 12", height, 120, 150, "red");

						send_command(0x00);

						pressed = true;
						CS_pin = 0;
						num_buttons--;
					}

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"GPIO 2") == 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();

					if (pressed) {

						Confirm_struct *params = (Confirm_struct*) malloc(
								sizeof(TaskParams));
						params->protocol = "SPI";
						params->val1 = CS_pin;
						params->val2 = 2;

						if (CS_pin == 12)
							params->val3 = 0;
						else
							params->val3 = 12;

						xTaskCreate(confirm, "confirm", 2048, (void*) params, 5,
								&main_menu_Handle);

						vTaskDelete(NULL);
					} else {
						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						print_ILI9488("X", 456, 0, 2);

						print_ILI9488("Select RX pin", 100, 0, 2);

						int height = 55;

						make_button("GPIO 0", height, 50, 60, "red");

						make_button("GPIO 12", height, 120, 150, "red");

						send_command(0x00);

						num_buttons--;
						pressed = true;
						CS_pin = 2;
						gpio_set_level(SS_display, 1);
					}

				}

				else if (strcmp(history_char[coord_index_char - i].app_name,
						"GPIO 12") == 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();
					if (pressed) {
						Confirm_struct *params = (Confirm_struct*) malloc(
								sizeof(TaskParams));
						params->protocol = "SPI";
						params->val1 = CS_pin;
						params->val2 = 12;

						if (CS_pin == 0)
							params->val3 = 2;
						else
							params->val3 = 0;

						xTaskCreate(confirm, "confirm", 2048, (void*) params, 5,
								&main_menu_Handle);

						vTaskDelete(NULL);

					} else {
						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);

						print_ILI9488("Select RX pin", 100, 0, 2);

						background_color = "red";

						int height = 55;

						make_button("GPIO 0", height, 50, 60, "red");

						make_button("GPIO 2", height, 240, 60, "red");

						send_command(0x00);

						num_buttons--;
						pressed = true;
						CS_pin = 12;
						gpio_set_level(SS_display, 1);
					}

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"close") == 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(GPIO_C_page_1, "GPIO_C_page_1", 2048,
					NULL, 5, &main_menu_Handle);
					vTaskDelete(NULL);
				}
			}
		}
	}
}

void GPIO_C_UART_page_0(void *pvParameters) {

	bool pressed = false;

	coord_index_char = 1;

	GPIO_pins_boot();

	gpio_set_level(SS_display, 0);
	print_ILI9488("Select TX pin", 100, 0, 2);
	send_command(0x00);
	gpio_set_level(SS_display, 1);

	uint8_t num_buttons = 4;

	uint8_t TX_pin = 0;

	while (1) {

		esp_task_wdt_reset();

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= num_buttons; i++) {

			if ((x >= history_char[coord_index_char - i].x
					&& x
							<= (history_char[coord_index_char - i].x
									+ history_char[coord_index_char - i].width))
					&& (y >= history_char[coord_index_char - i].y
							&& y
									<= (history_char[coord_index_char - i].y
											+ history_char[coord_index_char - i].height))) {

				if (strcmp(history_char[coord_index_char - i].app_name,
						"GPIO 0") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					if (pressed) {

						Confirm_struct *params = (Confirm_struct*) malloc(
								sizeof(TaskParams));
						params->protocol = "uart";
						params->val1 = TX_pin;
						params->val2 = 0;

						xTaskCreate(confirm, "confirm", 2048, (void*) params, 5,
								&main_menu_Handle);

						vTaskDelete(NULL);

					} else {

						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);

						print_ILI9488("Select RX pin", 100, 0, 2);

						background_color = "red";

						int height = 55;

						make_button("GPIO 2", height, 240, 60, "red");

						make_button("GPIO 12", height, 120, 150, "red");

						send_command(0x00);

						pressed = true;
						TX_pin = 0;
						num_buttons--;
					}

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"GPIO 2") == 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();

					if (pressed) {

						Confirm_struct *params = (Confirm_struct*) malloc(
								sizeof(TaskParams));
						params->protocol = "uart";
						params->val1 = TX_pin;
						params->val2 = 2;

						xTaskCreate(confirm, "confirm", 2048, (void*) params, 5,
								&main_menu_Handle);

						vTaskDelete(NULL);
					} else {
						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						print_ILI9488("X", 456, 0, 2);

						print_ILI9488("Select RX pin", 100, 0, 2);

						int height = 55;

						make_button("GPIO 0", height, 50, 60, "red");

						make_button("GPIO 12", height, 120, 150, "red");

						send_command(0x00);

						num_buttons--;
						pressed = true;
						TX_pin = 2;
						gpio_set_level(SS_display, 1);
					}

				}

				else if (strcmp(history_char[coord_index_char - i].app_name,
						"GPIO 12") == 0) {
					gpio_set_level(SS_display, 0);

					clean_screen();
					if (pressed) {
						Confirm_struct *params = (Confirm_struct*) malloc(
								sizeof(TaskParams));
						params->protocol = "uart";
						params->val1 = TX_pin;
						params->val2 = 12;

						xTaskCreate(confirm, "confirm", 2048, (void*) params, 5,
								&main_menu_Handle);

						vTaskDelete(NULL);

					} else {
						coord_index_char = 1;

						strcpy(history_char[coord_index_char].app_name,
								"close");
						history_char[coord_index_char].x = 456;
						history_char[coord_index_char].y = 0;
						history_char[coord_index_char].width = 24;
						history_char[coord_index_char].height = 29;

						coord_index_char++;

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);

						print_ILI9488("Select RX pin", 100, 0, 2);

						background_color = "red";

						int height = 55;

						make_button("GPIO 0", height, 50, 60, "red");

						make_button("GPIO 2", height, 240, 60, "red");

						send_command(0x00);

						num_buttons--;
						pressed = true;
						TX_pin = 12;
						gpio_set_level(SS_display, 1);
					}

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"close") == 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(GPIO_C_page_1, "GPIO_C_page_1", 2048,
					NULL, 5, &main_menu_Handle);
					vTaskDelete(NULL);
				}
			}
		}
	}
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
		fflush(stdout);  // Ensure it's flushed
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

	make_button("Easy", height, x, 30, "red");

	make_button("Medium", height, x, 120, "red");

	make_button("Chameya Mabloula!", height, x, 210, "red");

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

	make_button("PvP", height, x, 30, "red");

	make_button("AI vs AI", height, x, 120, "red");

	make_button("Player vs AI", height, x, 210, "red");

	send_command(0x00);

	gpio_set_level(SS_display, 1);

}

void pong_gamePvP_task(void *pvParameters) {

	int default_ball_speed_x = 10;   // Ball speed horizontally
	int default_ball_speed_y = 6;
	int paddle_speed = 4;          // Default paddle speed (will be updated)

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
							2048,
							NULL, 5, &main_menu_Handle);
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

			xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task", 2048,
			NULL, 5, &main_menu_Handle);

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
	int paddle_speed = 4;          // Default paddle speed (will be updated)
	int ai_paddle_speed = 4;       // Default paddle speed (will be updated)

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
							2048,
							NULL, 5, &main_menu_Handle);
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

			xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task", 2048,
			NULL, 5, &main_menu_Handle);

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
	int paddle_speed = 4;          // Default paddle speed (will be updated)

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
							2048,
							NULL, 5, &main_menu_Handle);
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

			xTaskCreate(pong_gamePage1_task, "pong_gamePage1_task", 2048,
			NULL, 5, &main_menu_Handle);

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

static void bootGoblin_slayer() {

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

	height = 50;
	x = 200;

	make_button("Start", height, x, 270, "red");

	send_command(0x00);

	gpio_set_level(SS_display, 1);

}

static void start_goblin_royale() {

	myMap.orientation = 'r';

	global_group = malloc(sizeof(goblin_group_t));

	for (int i = 0; i < goblin_nb; i++) {
		global_group->goblins[i] = &goblins[i];
	}

	for (int i = 0; i < goblin_nb; i++) {
		char taskName[16];
		snprintf(taskName, sizeof(taskName), "Goblin%dTask", i + 1);
		xTaskCreate(goblin_task, taskName, 1000, &goblins[i], 5,
				&goblinHandles[i]);
		vTaskSuspend(goblinHandles[i]);
	}

	vTaskResume(goblinHandles[0]); // Resume first goblin only
}

static void goblin_task_start(void *pvParameters) {
	coord_index_char = 1;
	bootGoblin_slayer();
	gpio_set_level(SS_display, 0);
	print_ILI9488("Touch the screen to place goblins (max is 10)", 0, 240, 1);
	send_command(0x00);
	gpio_set_level(SS_display, 1);

	while (1) {

		vTaskDelay(100);

		uint16_t x, y;
		if (!calculate_x_y(&x, &y)) {
			continue;
		}

		for (uint8_t i = 1; i <= 3; i++) {
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
					goblin_nb = 0;
					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"Start") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();
					send_command(0x00);

					coord_index_char = 1;

					start_goblin_royale();

					vTaskDelete(NULL);

				}
			}
			send_command(0x00);
			gpio_set_level(SS_display, 1);

		}
		if (goblin_nb < NUM_GOBLINS) {
			// Adjust coordinates to be multiples of 12 and 8
			x = x - (x % 12); // Snap to nearest lower multiple of 12
			y = y - (y % 8);  // Snap to nearest lower multiple of 8

			// Fill goblin data
			goblins[goblin_nb].xp = x;
			goblins[goblin_nb].yp = y;
			goblins[goblin_nb].real_xp = x;
			goblins[goblin_nb].real_yp = y;
			goblins[goblin_nb].orientation = 'r'; // Default orientation: right
			goblins[goblin_nb].health = 100;
			goblins[goblin_nb].last_draw_coord_index = 0;

			// Copy name safely
			strncpy(goblins[goblin_nb].name, goblinNames[goblin_nb],
					sizeof(goblins[goblin_nb].name) - 1);
			goblins[goblin_nb].name[sizeof(goblins[goblin_nb].name) - 1] = '\0';

			// Now render the goblin
			gpio_set_level(SS_display, 0);

			set_resolution_pos(x, y, 20, 20, 0);

			send_command(0x3A); // Interface pixel format
			send_ILI9488_data(0x06);

			send_command(0x2C);

			for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
				send_ILI9488_data(goblin_torch_run0[i]);
			}

			gpio_set_level(SS_display, 1);

			// Increment after everything
			goblin_nb++;
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

	make_button("Edit", 100, 40, 100, "red");
	make_button("Delete file", 100, 200, 100, "red");

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

			make_button(line, height, x, y, "red");

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

			make_button(line, height, x, y, "red");

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

					free(contents_local); // Don't forget to free the memory!

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

void GPIO_C_page_1(void *pvParameters) {

	coord_index_char = 1;

	GPIO_C_boot();

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
						"UART") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					xTaskCreate(GPIO_C_UART_page_0, "GPIO_C_UART_page_0", 2048,
					NULL, 5, &other_task_handel);

					vTaskDelete(NULL);

				} else if (strcmp(history_char[coord_index_char - i].app_name,
						"SPI") == 0) {

					gpio_set_level(SS_display, 0);

					clean_screen();

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					xTaskCreate(GPIO_C_SPI_page_0, "GPIO_C_SPI_page_0", 2048,
					NULL, 5, &other_task_handel);

					vTaskDelete(NULL);

				} else {

					gpio_set_level(SS_display, 0);

					clean_screen();

					send_command(0x00);
					gpio_set_level(SS_display, 1);

					xTaskCreate(GPIO_C_I2C_page_0, "GPIO_C_I2C_page_0", 2048,
					NULL, 5, &other_task_handel);

					vTaskDelete(NULL);

				}

			}
			send_command(0x00);
			gpio_set_level(SS_display, 1);

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

		for (uint8_t i = 0; i <= 4; i++) {
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

				} else if (strcmp(history[coord_index - i].app_name,
						"goblin_royale") == 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(goblin_task_start, "goblin_task_start", 2048,
					NULL, 5, &main_menu_Handle);

					vTaskDelete(NULL);

				}

				else if (strcmp(history[coord_index - i].app_name, "GPIO_C")
						== 0) {
					gpio_set_level(SS_display, 0);
					clean_screen();
					gpio_set_level(SS_display, 1);

					xTaskCreate(GPIO_C_page_1, "GPIO_C_page_1", 2048,
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

static void step_func_settings_check(char direction, goblin_torch *goblin) {

	if ((direction == 'd') || (direction == 'e') || (direction == 'c')) {
		if (goblin->orientation == 'l') {
			goblin->xp = 480 - goblin->xp - 20;
			goblin->orientation = 'r';
		}
		myMap.orientation = 'r';
		set_orientation(1);

	} else if ((direction == 'q') || (direction == 'a') || (direction == 'w')) {

		if (goblin->orientation == 'r') {
			goblin->xp = 480 - goblin->xp - 20;
			goblin->orientation = 'l';
		}
		myMap.orientation = 'l';
		set_orientation(5);

	} else {

		if (myMap.orientation == 'r') {
			if (goblin->orientation == 'l') {
				myMap.orientation = 'l';
				set_orientation(5);
			}
		} else if (myMap.orientation == 'l') {
			if (goblin->orientation == 'r') {
				myMap.orientation = 'r';
				set_orientation(1);
			}
		}

	}

}

void step_func_handle_tasks() {

	esp_task_wdt_reset();

	for (int i = 0; i < goblin_nb; i++) {

		current_goblin_index = (current_goblin_index + 1) % goblin_nb;

		if (goblinHandles[current_goblin_index] != NULL) {
			vTaskResume(goblinHandles[current_goblin_index]);
			break;
		}
	}

	if (death_bool)
		vTaskSuspend(NULL);

}

static void goblin_torch_step(char direction, goblin_torch *goblin,
		char enemy_id) {

	int x1 = 0;
	int y1 = 0;
	int delay = 1;
	if (direction == 'd') {
		x1 = 3;
		y1 = 0;

		goblin->real_xp += (x1 * 4);

		if (goblin->orientation == 'l') {
			goblin->xp = 480 - goblin->xp - 20;
			goblin->orientation = 'r';
		}
		myMap.orientation = 'r';
		set_orientation(1);

	} else if (direction == 'q') {
		x1 = 3;
		y1 = 0;

		goblin->real_xp -= (x1 * 4);

		if (goblin->orientation == 'r') {
			goblin->xp = 480 - goblin->xp - 20;
			goblin->orientation = 'l';
		}

		myMap.orientation = 'l';
		set_orientation(5);

	} else if (direction == 's') {
		x1 = 0;
		y1 = 2;
		goblin->real_yp += (y1 * 4);

		if (myMap.orientation == 'r') {
			if (goblin->orientation == 'l') {
				myMap.orientation = 'l';
				set_orientation(5);
			}
		} else if (myMap.orientation == 'l') {
			if (goblin->orientation == 'r') {
				myMap.orientation = 'r';
				set_orientation(1);
			}
		}

	} else if (direction == 'z') {
		x1 = 0;
		y1 = -2;
		goblin->real_yp += (y1 * 4);

		if (myMap.orientation == 'r') {
			if (goblin->orientation == 'l') {
				myMap.orientation = 'l';
				set_orientation(5);
			}
		} else if (myMap.orientation == 'l') {
			if (goblin->orientation == 'r') {
				myMap.orientation = 'r';
				set_orientation(1);
			}
		}

	} else if (direction == 'e') {
		x1 = 3;
		y1 = -2;
		goblin->real_xp += (x1 * 4);
		goblin->real_yp += (y1 * 4);

		if (goblin->orientation == 'l') {
			goblin->xp = 480 - goblin->xp - 20;
			goblin->orientation = 'r';
		}
		myMap.orientation = 'r';
		set_orientation(1);
	} else if (direction == 'a') {
		x1 = 3;
		y1 = -2;
		goblin->real_xp -= (x1 * 4);
		goblin->real_yp += (y1 * 4);

		if (goblin->orientation == 'r') {
			goblin->xp = 480 - goblin->xp - 20;
			goblin->orientation = 'l';
		}

		myMap.orientation = 'l';
		set_orientation(5);
	} else if (direction == 'w') {
		x1 = 3;
		y1 = 2;
		goblin->real_xp -= (x1 * 4);
		goblin->real_yp += (y1 * 4);

		if (goblin->orientation == 'r') {
			goblin->xp = 480 - goblin->xp - 20;
			goblin->orientation = 'l';
		}

		myMap.orientation = 'l';
		set_orientation(5);

	} else if (direction == 'c') {
		x1 = 3;
		y1 = 2;
		goblin->real_xp += (x1 * 4);
		goblin->real_yp += (y1 * 4);

		if (goblin->orientation == 'l') {
			goblin->xp = 480 - goblin->xp - 20;
			goblin->orientation = 'r';
		}
		myMap.orientation = 'r';
		set_orientation(1);
	}

	if ((goblin->xp + x1) > 480 || (goblin->xp + x1) < 0
			|| (goblin->yp + y1) > 320 || (goblin->yp + y1) < 0) {
		return;
	}

	goblin->xp += x1;
	goblin->yp += y1;

	goblin->last_draw_coord_index = coord_index;

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_run0[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	if ((goblin->xp + x1) > 480 || (goblin->xp + x1) < 0
			|| (goblin->yp + y1) > 320 || (goblin->yp + y1) < 0) {
		return;
	}

	goblin->xp += x1;
	goblin->yp += y1;

	step_func_handle_tasks();

	step_func_settings_check(direction, goblin);

	clean_last_element_modified(goblin->last_draw_coord_index);

	goblin->last_draw_coord_index = coord_index;

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_run1[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	if ((goblin->xp + x1) > 480 || (goblin->xp + x1) < 0
			|| (goblin->yp + y1) > 320 || (goblin->yp + y1) < 0) {
		return;
	}

	goblin->xp += x1;
	goblin->yp += y1;

	step_func_handle_tasks();

	step_func_settings_check(direction, goblin);

	clean_last_element_modified(goblin->last_draw_coord_index);

	goblin->last_draw_coord_index = coord_index;

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_run1[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	if ((goblin->xp + x1) > 480 || (goblin->xp + x1) < 0
			|| (goblin->yp + y1) > 320 || (goblin->yp + y1) < 0) {
		return;
	}

	goblin->xp += x1;
	goblin->yp += y1;

	printf("Draw goblin_torch_run2 done ! \n");

	step_func_handle_tasks();

	step_func_settings_check(direction, goblin);

	clean_last_element_modified(goblin->last_draw_coord_index);

	goblin->last_draw_coord_index = coord_index;

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_run2[i]);
	}
	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	if ((goblin->xp + x1) > 480 || (goblin->xp + x1) < 0
			|| (goblin->yp + y1) > 320 || (goblin->yp + y1) < 0) {
		return;
	}

	goblin->xp += x1;
	goblin->yp += y1;

	step_func_handle_tasks();

	step_func_settings_check(direction, goblin);

	clean_last_element_modified(goblin->last_draw_coord_index);

	goblin->last_draw_coord_index = coord_index;

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_run3[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	step_func_settings_check(direction, goblin);

	clean_last_element_modified(goblin->last_draw_coord_index);

	goblin->last_draw_coord_index = coord_index;

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_run0[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	goblin->xp -= x1;
	goblin->yp -= y1;

	step_func_handle_tasks();

	clean_last_element_modified(goblin->last_draw_coord_index);

}

void goblin_torch_death(char direction, goblin_torch *goblin) {

	uint64_t delay = 1;

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_run0[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_death1[i]);
	}
	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_death2[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_death3[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_death4[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_death5[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	goblin->last_draw_coord_index = coord_index;

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_death6[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

}

void goblin_torch_attack(char direction, goblin_torch *goblin, char enemy_id) {

	uint64_t delay = 1;

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);
	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_attack1[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_attack2[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_attack3[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_attack4[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}
	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0); // remove

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_attack5[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}
	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_attack4[i]);
	}
	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_attack3[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_attack2[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}
	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}
	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_attack1[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}
	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	goblin->last_draw_coord_index = coord_index;

	set_resolution_pos(goblin->xp, goblin->yp, 20, 20, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_goblin_torch_run0; i++) {
		send_ILI9488_data(goblin_torch_run0[i]);
	}

	for (uint64_t i = 0; i < delay; i++) {
		ets_delay_us(1);
	}

	step_func_handle_tasks();

	if (direction == 'r') {
		char direction = 'd';
		step_func_settings_check(direction, goblin);
	} else if (direction == 'l') {
		char direction = 'q';
		step_func_settings_check(direction, goblin);
	}

	clean_last_element_modified(goblin->last_draw_coord_index);

}

bool in_attack_range(goblin_torch *g1, goblin_torch *g2) {

	int dx = abs(g1->real_xp - g2->real_xp);
	int dy = abs(g1->real_yp - g2->real_yp);

	return dx <= ATTACK_RANGE && dy <= ATTACK_RANGE;
}

char get_move_direction(int dx, int dy) {
	if (dx > 0 && dy == 0)
		return 'd';   // right
	if (dx < 0 && dy == 0)
		return 'q';   // left
	if (dx == 0 && dy < 0)
		return 'z';   // up
	if (dx == 0 && dy > 0)
		return 's';   // down
	if (dx < 0 && dy < 0)
		return 'a';    // up-left
	if (dx > 0 && dy < 0)
		return 'e';    // up-right
	if (dx > 0 && dy > 0)
		return 'c';    // down-right
	if (dx < 0 && dy > 0)
		return 'w';    // down-left
	return 'n'; // default
}

void move_towards_enemy(goblin_torch *self, goblin_torch *enemy, char enemy_id) {
	int dx = enemy->real_xp - self->real_xp;
	int dy = enemy->real_yp - self->real_yp;

	char dir = get_move_direction(dx, dy);

	goblin_torch_step(dir, self, enemy_id);
}

goblin_torch* find_nearest_enemy(goblin_torch *self) {
	goblin_torch *nearest = NULL;
	int min_dist = INT_MAX;

	for (int i = 0; i < goblin_nb; i++) {
		goblin_torch *other = global_group->goblins[i];
		if (other == self || other->health <= 0)
			continue;

		int dx = other->real_xp - self->real_xp;
		int dy = other->real_yp - self->real_yp;
		int dist = dx * dx + dy * dy; // squared distance

		if (dist < min_dist) {
			min_dist = dist;
			nearest = other;
		}
	}

	return nearest;
}

void clean_task(void *params) {
	vTaskDelete(goblinHandles[current_goblin_index]);
	goblinHandles[current_goblin_index] = NULL;
	step_func_handle_tasks();
	death_bool = true;
	vTaskDelete(NULL);

}

void goblin_task(void *params) {
	goblin_torch *goblin = (goblin_torch*) params;

	while (1) {

		printf("\n---------------------------------------- start \n");
		printf("%s position -> x: %d, y: %d, health: %d\n", goblin->name,
				goblin->real_xp, goblin->real_yp, goblin->health);

		// Find the nearest enemy that's alive
		goblin_torch *enemy = find_nearest_enemy(goblin);

		if (!in_attack_range(goblin, enemy) && goblin->health > 0) {
			printf("%s moving toward %s\n", goblin->name, enemy->name);
			move_towards_enemy(goblin, enemy, enemy->name[1]);
		} else if (goblin->health > 0) {

			printf("%s attacking %s\n", goblin->name, enemy->name);
			goblin_torch_attack(goblin->orientation, goblin, enemy->name[1]);

			// Search for the goblin in the global group by name and reduce health by 10
			for (int i = 0; i < goblin_nb; i++) {
				if (strcmp(global_group->goblins[i]->name, enemy->name) == 0) {
					global_group->goblins[i]->health -= 10;
					if (global_group->goblins[i]->health < 0) {
						global_group->goblins[i]->health = 0;
					}
					break;
				}
			}

		} else {
			goblin_torch_death(goblin->orientation, goblin);
			death_bool = false;
			xTaskCreate(clean_task, "clean_task", 512, NULL,
			configMAX_PRIORITIES,
			NULL);
		}

		printf("\n---------------------------------------- end \n");

		esp_task_wdt_reset();

	}

}

