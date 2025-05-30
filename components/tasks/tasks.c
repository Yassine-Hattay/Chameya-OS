/**
 * @file tasks.c
 * @author your name (you@domain.com)
 * @brief this file implements the main tasks for the OS .
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "tasks.h"


bool Read_write_bit_i2c;

typedef struct {
	const char *protocol;
	uint8_t val1;
	uint8_t val2;
	uint8_t val3;
} Confirm_struct;

goblin_group_t *global_group = NULL;

/**
 * @brief Defines scaling factors and cell sizes for touch screen coordinate conversion.
 *
 * These static variables are used to translate raw touch screen input into
 * meaningful X and Y coordinates.
 * - `scaling_factor`: A factor applied to raw touch data, influencing the perceived
 * "speed of growth" or sensitivity of the touch input.
 * - `cell_size`: A conversion factor specific to the X-axis, determining the
 * real-world size represented by each unit of raw X-coordinate data.
 * - `cell_size1`: A conversion factor specific to the Y-axis, determining the
 * real-world size represented by each unit of raw Y-coordinate data.
 */

float scaling_factor = 1.013; // Adjust this for the desired speed of growth
float cell_size = 1.875; 
float cell_size1 = 1.25; 

TaskHandle_t main_menu_Handle = NULL;
TaskHandle_t other_task_handel = NULL;

char keyboard_buffer[MAX_COORDS_CHAR];
uint8_t keyboard_buffer_i = 0;

/**
 * @brief Defines the dimensions and initial positions of game elements for a Pong game.
 *
 * This section declares static variables that set up the visual and physical properties
 * of the paddles and the ball. It includes:
 * - `paddle_width`: The width of both the left and right paddles.
 * - `paddle_height`: The height of both the left and right paddles.
 * - `left_paddle_x`, `left_paddle_y`: The initial X and Y coordinates for the left paddle.
 * - `right_paddle_x`, `right_paddle_y`: The initial X and Y coordinates for the right paddle.
 * - `ball_x`, `ball_y`: The initial X and Y coordinates for the ball.
 * - `ball_size`: The size (diameter or side length) of the ball.
 */

static int paddle_width = 10;
static int paddle_height = 50;

static int left_paddle_x = 10;
static int left_paddle_y = 120;

static int right_paddle_x = 460;  // 480 - paddle_width - some margin
static int right_paddle_y = 120;

static int ball_x = 240;
static int ball_y = 160;
static int ball_size = 15;

static bool death_bool = true;

goblin_torch goblins[NUM_GOBLINS];

map myMap;

/**
 * @brief Array to hold task handles for each goblin.
 *
 * This array is used to store the FreeRTOS `TaskHandle_t` for each
 * goblin task, allowing for management and control of individual
 * goblin tasks. `NUM_GOBLINS` defines the total number of goblins.
 */

TaskHandle_t goblinHandles[NUM_GOBLINS];

static uint8_t current_goblin_index = 0;

// goblin_torch goblins[NUM_GOBLINS];
const char *goblinNames[NUM_GOBLINS] = { "G1", "G2", "G3", "G4", "G5", "G6",
		"G7", "G8", "G9", "G10" };

static int goblin_nb = 0;

/**
 * @brief Calculates the X and Y coordinates based on touch panel data.
 *
 * This function communicates with a touch panel controller via SPI to retrieve raw touch data,
 * then processes this data to calculate the X and Y coordinates. It sends control bytes
 * to acquire specific data, reads the responses, and applies a scaling factor and cell size
 * to convert the raw values into meaningful coordinates.
 *
 * @param x A pointer to a uint16_t where the calculated X coordinate will be stored.
 * @param y A pointer to a uint16_t where the calculated Y coordinate will be stored.
 * @return true if the coordinates are successfully calculated and are valid, false otherwise.
 */

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

/**
 * @brief Interrupt Service Routine (ISR) for the Pen Interrupt Request (PIRQ).
 *
 * This ISR is triggered by a pen interrupt, typically from a touch controller.
 * It notifies one or both of two FreeRTOS tasks (`main_menu_Handle` and `other_task_handel`)
 * if their respective task handles are valid. The `IRAM_ATTR` attribute ensures
 * the ISR is placed in IRAM for fast execution.
 * `portYIELD_FROM_ISR()` is called to request a context switch if a higher priority
 * task was unblocked by the notification.
 *
 * @param arg A pointer to arguments passed to the ISR, but it is not used in this function.
 */

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

/**
 * @brief FreeRTOS task for confirming communication parameters and initiating subsequent actions.
 *
 * This task displays a confirmation screen based on the communication protocol (UART, I2C, or SPI)
 * and the provided parameters (`val1`, `val2`, `val3`). It then waits for user input (touch)
 * to either proceed with data transmission/reception or return to a previous menu.
 *
 * For UART, it configures the TX pin and launches a keyboard task for transmission.
 * For I2C, it configures SDA/SCL pins and launches a keyboard task to enter a slave address.
 * For SPI, it configures MOSI, SCK, and SS pins and launches a keyboard task for transmission.
 *
 * If a "Transmit" area is touched, it sets up the relevant GPIOs and transitions to a
 * keyboard input task for sending data over the selected protocol.
 * If a "close" area is touched, it cleans the display and navigates back to the
 * appropriate protocol configuration page (UART, I2C, or SPI).
 *
 * @param pvParameters A pointer to a `Confirm_struct` containing the protocol type and
 * associated configuration values.
 */

static void confirm(void *pvParameters) {
	Confirm_struct *data = (Confirm_struct*) pvParameters;

	coord_index_char = 1;

	background_color = "black";

	char buffer[16];  // Safe for "TX:255" + '\0'

	if (strcmp(data->protocol, "uart") == 0) {
		gpio_set_level(SS_display, 0);

		background_color = "black";

		sprintf(buffer, "TX:%d", data->val1);
		print_ILI9488(buffer, 10, 50, 2);
		sprintf(buffer, "RX:%d", data->val2);
		print_ILI9488(buffer, 200, 50, 2);

		print_ILI9488("confirm ?", 100, 0, 2);
		confirm_boot();

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

		sprintf(buffer, "CS:%d", data->val1);
		background_color = "black";

		print_ILI9488(buffer, 10, 50, 2);
		sprintf(buffer, "SCK:%d", data->val2);
		print_ILI9488(buffer, 160, 50, 2);

		sprintf(buffer, "MOSI:%d", data->val3);
		print_ILI9488(buffer, 305, 50, 2);

		print_ILI9488("confirm ?", 100, 0, 2);
		confirm_boot();

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
						background_color = "black";

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

						gpio_config_t io_conf = { .pin_bit_mask =
								(1ULL << BMOSI) | (1ULL << BSCK)
										| (1ULL << BSS), .mode =
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

					if (strcmp(data->protocol, "SPI") == 0) {
						xTaskCreate(GPIO_C_SPI_page_0, "GPIO_C_UART_page_0",
								2048,
								NULL, 5, &main_menu_Handle);

						vTaskDelete(NULL);

					}
				}
			}
		}
	}
}

/**
 * @brief FreeRTOS task for configuring I2C communication parameters.
 *
 * This task presents a menu to the user for selecting the SDA and SCL pins
 * to be used for I2C communication. It initializes the GPIO pins using
 * `GPIO_pins_boot()`, then prompts the user to select the SDA pin.
 *
 * Upon selecting an SDA pin, it prompts the user to select the SCL pin.
 * After both pins are chosen, it presents options to either "Write" or "Read"
 * using the configured I2C interface.
 *
 * The task uses touch input to detect user selections. The coordinates
 * of the touch are compared against predefined button areas.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 */

static void GPIO_C_I2C_page_0(void *pvParameters) {
	char buffer[16];

	bool pressed = false;

	coord_index_char = 1;

	Confirm_struct *params = (Confirm_struct*) malloc(sizeof(TaskParams));

	GPIO_pins_boot();

	gpio_set_level(SS_display, 0);
	background_color = "black";

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

						background_color = "black";

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

						background_color = "black";

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
						background_color = "black";

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
						background_color = "black";

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

						background_color = "black";

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
						background_color = "black";

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

/**
 * @brief FreeRTOS task for configuring SPI communication parameters.
 *
 * This task guides the user through selecting the Chip Select (CS),
 * Serial Clock (SCK), and Master Out Slave In (MOSI) pins for SPI communication.
 * It initializes the GPIO pins using `GPIO_pins_boot()` and displays prompts
 * on the ILI9488 display.
 *
 * The user interacts with the display via touch input, selecting the desired
 * GPIO pins for each SPI function. After all pins are selected, it creates a
 * `Confirm_struct` with the chosen parameters and transitions to the `confirm` task
 * to finalize the setup or return to the previous menu.
 * The task continuously monitors for touch input and resets the watchdog timer.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 */

void GPIO_C_SPI_page_0(void *pvParameters) {
	bool pressed = false;

	coord_index_char = 1;

	GPIO_pins_boot();

	gpio_set_level(SS_display, 0);
	background_color = "black";
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
						background_color = "black";

						print_ILI9488("Select SCK pin", 100, 0, 2);

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

						background_color = "red";
						print_ILI9488("X", 456, 0, 2);
						background_color = "black";

						print_ILI9488("Select SCK pin", 100, 0, 2);

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
						background_color = "black";

						print_ILI9488("Select SCK pin", 100, 0, 2);

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

/**
 * @brief FreeRTOS task for configuring UART communication parameters.
 *
 * This task guides the user through selecting the Transmit (TX) and Receive (RX) pins
 * for UART communication via a touch-based interface on the ILI9488 display.
 * It begins by initializing GPIO pins and prompting the user to select the TX pin.
 *
 * Once a TX pin is chosen, the display clears and prompts for the RX pin.
 * After both TX and RX pins are selected, a `Confirm_struct` is populated with the
 * chosen pins and the task transitions to the `confirm` task to finalize the UART setup
 * or to return to the previous menu.
 * The task continuously monitors for touch input and ensures the watchdog timer is reset
 * to prevent system resets due to inactivity.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 */

void GPIO_C_UART_page_0(void *pvParameters) {

	bool pressed = false;

	coord_index_char = 1;

	GPIO_pins_boot();

	gpio_set_level(SS_display, 0);
	background_color = "black";

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
						background_color = "black";

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
						background_color = "black";

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
						background_color = "black";

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

/**
 * @brief Clamps a float value within a specified minimum and maximum range.
 *
 * This function takes a floating-point value and ensures it stays within
 * the bounds defined by `min` and `max`. If `val` is less than `min`,
 * it returns `min`. If `val` is greater than `max`, it returns `max`.
 * Otherwise, it returns `val` itself.
 *
 * @param val The float value to clamp.
 * @param min The minimum allowed float value.
 * @param max The maximum allowed float value.
 * @return The clamped float value.
 */

static float clampf(float val, float min, float max) {
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}

/**
 * @brief FreeRTOS task to monitor and print the available free heap memory.
 *
 * This task continuously retrieves the current free heap size using `esp_get_free_heap_size()`
 * and prints it to the console. It then flushes the output buffer to ensure immediate display.
 * The task delays for 500 milliseconds (0.5 seconds) between each measurement, effectively
 * printing the free heap size twice per second.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 */

void memory_monitor_task(void *pvParameters) {
	while (1) {
		printf("Current free heap: %u bytes\n", esp_get_free_heap_size());
		fflush(stdout);  // Ensure it's flushed
		vTaskDelay(500);  // Every 5 seconds
	}
}

/**
 * @brief Initializes and displays the main menu for the "Pong" game.
 *
 * This function sets up the initial display for the Pong game, presenting
 * difficulty options to the user. It configures a "close" button to exit the game,
 * and three difficulty buttons: "Easy", "Medium", and "Chameya Mabloula!"
 * (likely a difficult or insane mode).
 *
 * It controls the ILI9488 display to clear the screen, set background colors,
 * and render the text and buttons at specified coordinates.
 *
 * @note This function only sets up the display and buttons; it does not handle
 * the button press logic, which is expected to be managed by a separate task
 * that monitors touch input and reacts to these defined button areas.
 */

static void bootApp_pong_B() {

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
/**
 * @brief Initializes and displays the game mode selection menu for Pong.
 *
 * This function sets up the initial screen for the Pong game, allowing the user
 * to select a game mode. It draws a "close" button to exit the application and
 * three game mode buttons: "PvP" (Player versus Player), "AI vs AI", and
 * "Player vs AI".
 *
 * The function first takes the display offline (`gpio_set_level(SS_display, 0)`),
 * updates the `history_char` array with the "close" button's properties,
 * and then draws all elements on the ILI9488 display with appropriate colors
 * and positions. Finally, it brings the display back online
 * (`gpio_set_level(SS_display, 1)`).
 *
 * @note This function is responsible only for rendering the UI elements.
 * The actual handling of button presses and game mode transitions would
 * occur in a different part of the code that monitors touch events.
 */
static void bootApp_pong() {

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


/**
 * @brief FreeRTOS task for the Player vs Player (PvP) mode of the Pong game.
 *
 * This task manages the entire game logic and display for a two-player Pong game.
 * It starts by presenting a difficulty selection menu ("Easy", "Medium", "Chameya Mabloula!")
 * via `bootApp_pong_B()`, allowing players to choose the ball and paddle speeds.
 *
 * After difficulty selection, it initializes game elements such as scores,
 * ball position and speed, and paddle positions. The game loop continuously:
 * - Resets the watchdog timer.
 * - Reads touch input to control the left and right paddles.
 * - Checks for a "close" button press to exit the game.
 * - Clamps paddle positions to stay within screen bounds.
 * - Updates ball position based on its speed.
 * - Handles ball collisions with top/bottom walls and paddles,
 * calculating bounce angles for realistic physics.
 * - Manages scoring when the ball goes past a paddle.
 * - Redraws the game state (ball, paddles, score) on the ILI9488 display,
 * clearing previous positions before drawing new ones to avoid artifacts.
 * - Introduces a delay to control the game speed.
 *
 * If the "close" button is pressed or a score limit (not explicitly defined in this snippet but implied by game flow)
 * is reached, the task cleans up the display and transitions back to the `pong_gamePage1_task`.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 */
static void pong_gamePvP_task(void *pvParameters) {

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
	print_ILI9488("X", 305, 0, 2);
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
/**
 * @brief FreeRTOS task for the Player vs. AI (PvA) mode of the Pong game.
 *
 * This task implements the game logic for a single player against an AI opponent in Pong.
 * It begins by presenting a difficulty selection menu ("Easy", "Medium", "Chameya Mabloula!")
 * using `bootApp_pong_B()`, which sets the initial ball and paddle speeds for both the player and the AI.
 *
 * After the difficulty is chosen, the game initializes scores, ball position and speed, and paddle positions.
 * The main game loop continuously performs the following actions:
 * - **Watchdog Reset:** Resets the hardware watchdog timer to prevent system reboots.
 * - **Touch Input (Player Paddle):** Detects touch input to control the left (player) paddle's vertical movement.
 * - **Exit Condition:** Checks for a "close" button press to exit the current game and return to the `pong_gamePage1_task`.
 * - **Paddle Clamping:** Ensures the player's paddle stays within the vertical bounds of the screen.
 * - **Ball Movement:** Updates the ball's position based on its current horizontal and vertical speeds.
 * - **Wall Collisions:** Reverses the ball's vertical speed if it hits the top or bottom walls.
 * - **Paddle Collisions:** Handles collisions with both the player's (left) and AI's (right) paddles. It calculates
 * a bounce angle to determine the new ball direction, adding some randomness for the AI's bounce.
 * - **Scoring:** Increments the player's or AI's score when the ball goes off-screen horizontally past a paddle.
 * The ball is then reset to the center, and its speed is adjusted for the next round.
 * - **AI Movement:** Controls the right (AI) paddle. The AI attempts to track the ball's vertical position,
 * with a slight random offset to make its movement less predictable. The AI paddle is also clamped to the screen bounds.
 * - **Rendering:** Clears the previous positions of the ball and paddles on the ILI9488 display, then draws
 * them in their new positions. The score is also updated on the display.
 * - **Game Speed Control:** Introduces a delay using `vTaskDelay()` to regulate the game's frame rate.
 *
 * When the task exits (e.g., via the "close" button), it performs necessary screen cleanup and transitions
 * to the `pong_gamePage1_task`.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 */
static void pong_gamePvA_task(void *pvParameters) {
	int default_ball_speed_x = 10;   // Ball speed horizontally
	int default_ball_speed_y = 6;
	int paddle_speed = 4;          // Default paddle speed (will be updated)
	int ai_paddle_speed = 4;       // Default paddle speed (will be updated)

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
/**
 * @brief FreeRTOS task for the Player vs. AI (PvA) mode of the Pong game.
 *
 * This task implements the game logic for a single player against an AI opponent in Pong.
 * It begins by presenting a difficulty selection menu ("Easy", "Medium", "Chameya Mabloula!")
 * using `bootApp_pong_B()`, which sets the initial ball and paddle speeds for both the player and the AI.
 *
 * After the difficulty is chosen, the game initializes scores, ball position and speed, and paddle positions.
 * The main game loop continuously performs the following actions:
 * - **Watchdog Reset:** Resets the hardware watchdog timer to prevent system reboots.
 * - **Touch Input (Player Paddle):** Detects touch input to control the left (player) paddle's vertical movement.
 * - **Exit Condition:** Checks for a "close" button press to exit the current game and return to the `pong_gamePage1_task`.
 * - **Paddle Clamping:** Ensures the player's paddle stays within the vertical bounds of the screen.
 * - **Ball Movement:** Updates the ball's position based on its current horizontal and vertical speeds.
 * - **Wall Collisions:** Reverses the ball's vertical speed if it hits the top or bottom walls.
 * - **Paddle Collisions:** Handles collisions with both the player's (left) and AI's (right) paddles. It calculates
 * a bounce angle to determine the new ball direction, adding some randomness for the AI's bounce.
 * - **Scoring:** Increments the player's or AI's score when the ball goes off-screen horizontally past a paddle.
 * The ball is then reset to the center, and its speed is adjusted for the next round.
 * - **AI Movement:** Controls the right (AI) paddle. The AI attempts to track the ball's vertical position,
 * with a slight random offset to make its movement less predictable. The AI paddle is also clamped to the screen bounds.
 * - **Rendering:** Clears the previous positions of the ball and paddles on the ILI9488 display, then draws
 * them in their new positions. The score is also updated on the display.
 * - **Game Speed Control:** Introduces a delay using `vTaskDelay()` to regulate the game's frame rate.
 *
 * When the task exits (e.g., via the "close" button), it performs necessary screen cleanup and transitions
 * to the `pong_gamePage1_task`.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 */
static void pong_gameAvA_task(void *pvParameters) {
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
/**
 * @brief FreeRTOS task for the first page of the Pong game, handling game mode selection.
 *
 * This task is responsible for displaying the initial Pong game menu and
 * managing user interaction to select a game mode (PvP, AI vs AI, or Player vs AI).
 * It uses `bootApp_pong()` to render the menu buttons on the ILI9488 display.
 *
 * The task runs in an infinite loop, continuously performing the following actions:
 * - **Watchdog Reset:** Resets the hardware watchdog timer to prevent system reboots due to inactivity.
 * - **Touch Input:** Reads touch coordinates from the display. If no valid touch is detected, it continues to the next iteration.
 * - **Button Detection:** Iterates through the stored button configurations (`history_char`) to check if the touch coordinates fall within any button's area.
 * - **Game Mode Selection:** Based on the detected button, it performs the following actions:
 * - **"close" button:** Clears the screen, deactivates the display SS line, and creates the `main_menu_task` before deleting itself.
 * - **"AI vs AI" button:** Clears the screen, deactivates the display SS line, and creates the `pong_gameAvA_task` before deleting itself.
 * - **"Player vs AI" button:** Clears the screen, deactivates the display SS line, and creates the `pong_gamePvA_task` before deleting itself.
 * - **"PvP" (Player vs Player) button (default else case):** Clears the screen, deactivates the display SS line, and creates the `pong_gamePvP_task` before deleting itself.
 * - **Display Update:** After processing a touch or in each loop iteration, it ensures the display command is sent and the display SS line is activated.
 *
 * This task effectively serves as a menu handler, transitioning to different Pong game
 * modes based on user input.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 */
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
/**
 * @brief Initializes and displays the main menu for the "Goblin Slayer" game.
 *
 * This function sets up the initial screen for the Goblin Slayer game, presenting
 * a "Start" button to begin the game and a "close" button to exit.
 *
 * It first disables the display's Slave Select (SS) line (`gpio_set_level(SS_display, 0)`)
 * to prepare for drawing. It then configures and records the properties of the "close" button
 * in the `history_char` array, which is likely used for touch input detection.
 * The "X" character for the close button is then printed on the ILI9488 display with a red background.
 *
 * Next, it sets up and draws the "Start" button, also with a red background.
 * Finally, it sends a command to the display (`send_command(0x00)`) and
 * re-enables the display's SS line (`gpio_set_level(SS_display, 1)`) to make the changes visible.
 *
 * @note This function is solely responsible for rendering the initial UI. The actual
 * game logic and handling of button presses (e.g., starting the game or exiting)
 * are expected to be implemented in a separate task or function that monitors touch events
 * and acts upon the `history_char` data.
 */
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
/**
 * @brief Initializes and starts the "Goblin Royale" game.
 *
 * This function sets up the game environment for "Goblin Royale". It performs the following steps:
 *
 * 1. **Map Orientation:** Sets the `orientation` of `myMap` to 'r'.
 * 2. **Global Goblin Group Allocation:** Dynamically allocates memory for a `goblin_group_t` structure,
 * assigning it to `global_group`. This structure likely holds pointers to all goblin entities in the game.
 * 3. **Goblin Pointer Population:** Populates the `goblins` array within `global_group` with pointers
 * to individual `goblins` (presumably an array of `goblin_t` structures defined elsewhere).
 * 4. **Goblin Task Creation and Suspension:** Iterates through the number of goblins (`goblin_nb`).
 * For each goblin, it creates a FreeRTOS task (`goblin_task`) responsible for that goblin's behavior.
 * Each task is given a unique name (e.g., "Goblin1Task", "Goblin2Task").
 * Immediately after creation, each goblin task is suspended using `vTaskSuspend()`, meaning they won't
 * start executing yet.
 * 5. **Resume First Goblin:** Resumes only the first goblin's task (`goblinHandles[0]`) using `vTaskResume()`,
 * implying that goblins might be introduced into the game sequentially or under specific conditions.
 *
 * This function prepares all goblin entities and their associated tasks, then strategically
 * starts the first goblin, hinting at a wave-based or progressive game mechanic.
 */
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
/**
 * @brief FreeRTOS task for the initial setup and goblin placement in "Goblin Royale".
 *
 * This task manages the pre-game phase of "Goblin Royale," where the player places goblins on the screen.
 * It first displays the main menu using `bootGoblin_slayer()` and then prompts the user to
 * touch the screen to place goblins, with a maximum limit of 10.
 *
 * The task continuously monitors for touch input and button presses:
 * - **Touch Input for Goblin Placement:** When the screen is touched and the number of placed goblins
 * is less than `NUM_GOBLINS`, it calculates snapped X and Y coordinates (multiples of 12 and 8, respectively).
 * It then initializes a new goblin's data (position, health, orientation, name) and draws it on the ILI9488 display
 * using `goblin_torch_run0` (likely an image data array for the goblin sprite). The `goblin_nb` counter is incremented.
 * - **Button Interaction:** It checks for presses on the "close" and "Start" buttons:
 * - **"close" button:** If pressed, it cleans the screen, resets the `goblin_nb` counter to 0,
 * and transitions back to the `main_menu_task` before deleting itself.
 * - **"Start" button:** If pressed, it cleans the screen, initializes and resumes the goblin tasks
 * via `start_goblin_royale()`, and then deletes itself, transferring control to the game logic.
 *
 * The task ensures proper display updates by controlling the `SS_display` GPIO level and sending
 * display commands. A `vTaskDelay(100)` is included to debounce touch inputs and control the loop's frequency.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 */
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
/**
 * @brief FreeRTOS task for handling a virtual keyboard display and input.
 *
 * This task manages a virtual keyboard presented on the display, allowing user
 * input. It receives initial positioning and task transition information
 * through its `pvParameters`.
 *
 * The task performs the following actions:
 * - **Initialization:**
 * - Casts `pvParameters` to `TaskParams*` to access initial `x`, `y`
 * coordinates, and names of the `previous_task` and `current_task`.
 * - Initializes `case_type` to 'l' (lowercase) for the keyboard.
 * - Calls `draw_keyborad()` to render the virtual keyboard on the display.
 * - **Main Loop:**
 * - **Delay:** Introduces a `vTaskDelay(100)` to debounce touch inputs and
 * control the loop's execution frequency.
 * - **Touch Input:** Attempts to calculate touch coordinates (`x`, `y`). If
 * no valid touch is detected, it continues to the next loop iteration.
 * - **Key Press Check:** Calls `check_key_press()` to process the touch
 * input. This function is responsible for:
 * - Determining which key was pressed based on `x` and `y`.
 * - Updating `x1` and `y1` (likely the cursor or text insertion point).
 * - Handling case changes (e.g., lowercase to uppercase) by modifying
 * `case_type`.
 * - Potentially transitioning to `previous_task` or `current_task`
 * based on special key presses (e.g., Enter, Back).
 * - Passing the `data` structure, implying it might update input fields
 * within `data`.
 * - **Display Update:** Sends a display command (`send_command(0x00)`) and
 * activates the display's Slave Select (SS) line (`gpio_set_level(SS_display, 1)`)
 * to ensure any changes from `check_key_press` are rendered.
 *
 * @param pvParameters A pointer to a `TaskParams` structure containing
 * initial keyboard display coordinates and task
 * transition information.
 */
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
/**
 * @brief FreeRTOS task for handling file editing and deletion options in the notebook application.
 *
 * This task presents a second-page menu for a selected notebook file, offering "Edit" and "Delete file" options,
 * along with a "close" (X) button to return to the previous file listing.
 *
 * @param pvParameters A pointer to a `char` array (string) containing the name of the file
 * that was selected on the previous page. The memory for this `filename` is expected to be
 * freed within this task before it deletes itself.
 *
 * The task's main loop continuously:
 * - **Resets Watchdog:** Calls `esp_task_wdt_reset()` to prevent the system watchdog from timing out.
 * - **Detects Touch:** Uses `calculate_x_y()` to get touch coordinates. If no touch is detected, it continues.
 * - **Handles Button Presses:** Iterates through the predefined button areas (`history_char`) to check if
 * a touch corresponds to a button:
 * - **"Delete file" button:**
 * - Constructs the full path to the file on the SPIFFS filesystem.
 * - Calls `delete_file()` to remove the file.
 * - Clears the display and the `history_char` buffer.
 * - Frees the `filename` memory.
 * - Creates and transitions to the `notebook_editFilesPage1_task` (the previous page).
 * - Deletes itself.
 * - **"close" (X) button:**
 * - Clears the display and the `history_char` buffer.
 * - Frees the `filename` memory.
 * - Creates and transitions to the `notebook_editFilesPage1_task`.
 * - Deletes itself.
 * - **"Edit" button:**
 * - Clears the display and the `history_char` buffer.
 * - Constructs the full path to the file.
 * - Reads the file's contents using `read_file_contents()`.
 * - Displays the filename as a title and then prints the file's contents character by character,
 * populating a `keyboard_buffer` with the content for editing.
 * - Allocates and populates a `TaskParams` structure with the current cursor position (`x1`, `y1`)
 * and task transition information ("notebook_editFilesPage2_task" as previous, "keyboard_to_edit" as current).
 * - Sets `paragraph_number` to 1 (likely for text formatting).
 * - Creates and transitions to the `keyboard_task` to allow editing.
 * - Frees the `filename` memory and the `contents_local` buffer.
 * - Deletes itself.
 *
 * Display operations (e.g., `gpio_set_level(SS_display, 0)`, `send_command(0x00)`) are managed to ensure
 * smooth screen updates.
 */
static void notebook_editFilesPage2_task(void *pvParameters) {

	char *filename = (char*) pvParameters;

	make_button("Edit", 80, 20, 100, "red");
	make_button("Delete file", 80, 180, 100, "red");

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
					background_color = "black";

					print_ILI9488(filename, 100, 0, 2);

					char *contents_local = read_file_contents(full_path);
					char *original_ptr = contents_local; // Save original pointer

					uint16_t x1 = 0, y1 = 35;

					keyboard_buffer_i = 0;
					coord_index_char = 1;

					while (*contents_local) {
						char c = *contents_local;

						background_color = "black";
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

/**
 * @brief FreeRTOS task for displaying and managing existing notebook files.
 *
 * This task presents a list of existing notebook files, allowing the user to select one
 * for further actions (editing or deleting) or to return to the main notebook application page.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 *
 * The task initializes the display by:
 * - Setting the display's Slave Select (SS) line low (`gpio_set_level(SS_display, 0)`).
 * - Printing the title "files" at the top of the screen.
 * - Setting the SS line high (`gpio_set_level(SS_display, 1)`).
 *
 * It then reads the list of filenames from `/spiffs/notebook/filenames.txt`.
 * If content is found:
 * - It parses the content line by line, treating each line as a filename.
 * - For each filename, it dynamically creates a button on the display. The button's width
 * is adjusted based on the length of the filename to ensure it fits the text.
 * - Buttons are arranged horizontally, wrapping to the next line if they exceed the display width.
 * - Each filename string is duplicated using `strdup()` and stored in the `filenames` array.
 *
 * A "close" (X) button is also created at the top right of the screen.
 *
 * The task then enters its main loop, continuously performing the following:
 * - **Watchdog Reset:** Resets the hardware watchdog timer (`esp_task_wdt_reset()`).
 * - **Touch Input:** Checks for touch input using `calculate_x_y()`. If no touch is detected,
 * it continues to the next iteration.
 * - **Button Interaction:** It iterates through all the created buttons (including the "close" button
 * and file buttons) to check if a touch corresponds to a button press:
 * - **File Button (not "close" and `x_level` is not 1):**
 * - Clears the screen and the `history_char` buffer.
 * - Prints the selected filename as a title on the next page.
 * - Creates a copy of the selected filename using `strdup()` to pass to the next task.
 * - Frees all the `filenames` memory allocated earlier.
 * - Creates and transitions to the `notebook_editFilesPage2_task`, passing the selected filename as a parameter.
 * - Sets `x_level` to 1 (likely a flag to prevent re-entering this condition).
 * - Deletes itself.
 * - **"close" button (`x_level` is 0):** This condition is likely intended for exiting the current page.
 * - Clears the screen and the `history_char` buffer.
 * - Frees the `content` buffer and all `filenames` memory.
 * - Creates and transitions to the `note_book_app_page1` task.
 * - Deletes itself.
 * - **"close" button (another `x_level` condition):** This block appears to handle returning to the file list from a sub-menu or a soft-reset of the page.
 * - Clears the screen and `history_char` buffer.
 * - Reprints the "files" title.
 * - Resets `x_level` to 0 and `coord_index_char` to 1.
 * - Redraws all the file buttons on the screen in their original positions.
 * - Recreates the "close" button.
 * - Updates the display and then `break`s from the loop to re-evaluate touches.
 *
 * Throughout the function, `gpio_set_level(SS_display, 0)` and `gpio_set_level(SS_display, 1)`
 * are used to control display updates, and `send_command(0x00)` is used for display synchronization.
 */
void notebook_editFilesPage1_task(void *pvParameters) {

	gpio_set_level(SS_display, 0);

	background_color = "black";
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

		char *line = strtok(content, "\n");

		while (line != NULL && file_count < MAX_FILES) {

			// Store a copy of the token
			filenames[file_count] = strdup(line); // allocates and copies the string

			int filename_length = strlen(filenames[file_count]); // Length of the filename
			width = 24 * filename_length + 24;  // Multiply the length by 24

			if (x + width > 420) {
				y += height + 5;
				x = 5;
			}
			background_color = "red";

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
					background_color = "black";

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

					background_color = "black";
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

/**
 * @brief FreeRTOS task for displaying and allowing the user to read existing notebook files.
 *
 * This task is responsible for presenting a list of notebook files stored on the device.
 * Users can select a file to view its contents or choose to exit back to the main notebook application page.
 *
 * @param pvParameters Unused parameter, required by FreeRTOS task signature.
 *
 * The task performs the following steps upon creation:
 * - **Display Initialization:** Sets the display's Slave Select (SS) line low, prints a "files" title,
 * and then sets the SS line high to update the display.
 * - **Read File List:** Reads the contents of `/spiffs/notebook/filenames.txt`, which is expected to contain
 * a newline-separated list of notebook file names.
 * - **Dynamic Button Creation:**
 * - If `filenames.txt` contains content, it parses each filename.
 * - For each filename, it dynamically creates a **red button** on the display. The button's width
 * is adjusted based on the length of the filename to ensure the text fits properly.
 * - Buttons are arranged horizontally, wrapping to the next line when they exceed the display width (420 pixels).
 * - Each filename is duplicated using `strdup()` and stored in a `filenames` array of pointers for later use.
 * - **Cleanup:** Frees the memory allocated for the `content` buffer obtained from `read_file_contents`.
 * - **"Close" Button:** Creates a standard "X" (close) button at the top right of the screen.
 *
 * The task then enters an infinite loop to handle user interactions:
 * - **Watchdog Reset:** Calls `esp_task_wdt_reset()` to prevent the system watchdog from timing out.
 * - **Touch Input:** Continuously checks for touch input using `calculate_x_y()`. If no touch is detected,
 * it skips to the next loop iteration.
 * - **Button Detection and Handling:** It iterates through all the created buttons (file buttons and the "close" button)
 * to determine if a touch falls within any button's area:
 * - **File Button (not "close" and `x_level` is not 1):**
 * - Clears the display screen.
 * - Prints the selected filename as a title at the top.
 * - Constructs the full path to the selected file (e.g., "/spiffs/notebook/filename.txt").
 * - Reads the actual contents of the selected file using `read_file_contents()`.
 * - Prints the contents of the file on the display.
 * - Frees the memory allocated for the file's contents.
 * - Sets `x_level` to 1, indicating that a file is currently being displayed.
 * - Updates the display and then `break`s from the button loop to wait for further interaction.
 * - **"Close" button (`x_level` is 0):** This condition handles exiting the file list view.
 * - Clears the display and the `history_char` buffer (which stores button coordinates).
 * - Frees the initial `content` buffer and all dynamically allocated `filenames`.
 * - Creates and transitions to the `note_book_app_page1` task.
 * - Deletes itself.
 * - **"Close" button (When `x_level` is 1):** This condition handles returning from viewing a file's content back to the file list.
 * - Clears the display and the `history_char` buffer.
 * - Reprints the "files" title.
 * - Resets `x_level` to 0 and `coord_index_char` to 1.
 * - Redraws all the file buttons on the screen in their original positions.
 * - Recreates the "X" (close) button.
 * - Updates the display and then `break`s from the button loop to allow re-selection of files.
 *
 * **Display Control:** Throughout the task, `gpio_set_level(SS_display, 0)` and `gpio_set_level(SS_display, 1)`
 * are used to control when the display is updated, and `send_command(0x00)` is used to ensure commands are sent.
 */

static void notebook_readfiles_task(void *pvParameters) {

	gpio_set_level(SS_display, 0);

	background_color = "black";
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

		char *line = strtok(content, "\n");

		while (line != NULL && file_count < MAX_FILES) {

			// Store a copy of the token
			filenames[file_count] = strdup(line); // allocates and copies the string

			int filename_length = strlen(filenames[file_count]); // Length of the filename
			width = 24 * filename_length + 24;  // Multiply the length by 24

			if (x + width > 420) {
				y += height + 5;
				x = 5;
			}
			background_color = "red";

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
					background_color = "black";

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

					background_color = "black";
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
						background_color = "red";

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
/**
 * @brief FreeRTOS task for the main menu of the GPIO Control (GPIO_C) app.
 *
 * This task displays the initial menu for the GPIO Control application, allowing users
 * to select between UART, SPI, I2C, or to exit to the main system menu.
 *
 * @param pvParameters Unused.
 *
 * The task initializes the menu using `GPIO_C_boot()`. It then enters a loop
 * to detect touch input. Upon a valid touch, it checks which button was pressed:
 * - **"close" button:** Transitions to `main_menu_task`.
 * - **"UART" button:** Transitions to `GPIO_C_UART_page_0`.
 * - **"SPI" button:** Transitions to `GPIO_C_SPI_page_0`.
 * - **Other (implicitly "I2C") button:** Transitions to `GPIO_C_I2C_page_0`.
 *
 * For all transitions, it clears the screen and creates the new task before
 * deleting itself. The display's SS line is managed for updates, and the
 * watchdog timer is periodically reset.
 */
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
/**
 * @brief FreeRTOS task for the main menu of the embedded system.
 *
 * This task displays the primary application icons and handles user selection
 * to launch different functionalities like a notebook, Pong game, Goblin Royale,
 * or GPIO Control.
 *
 * @param pvParameters Unused.
 *
 * The task starts by drawing the main menu icons via `draw_main_menu_icons()`.
 * It then enters an infinite loop, continuously monitoring for user touch input:
 * - **Watchdog Reset:** Resets the hardware watchdog timer (`esp_task_wdt_reset()`).
 * - **Touch Input:** Reads touch coordinates (`x`, `y`). If no touch is detected, it continues.
 * - **Application Launch:** Iterates through the stored icon data (`history`) to check
 * if a touch corresponds to an application icon:
 * - **"notebook":** Clears the screen and creates `note_book_app_page1` task.
 * - **"pong":** Clears the screen and creates `pong_gamePage1_task` task.
 * - **"goblin_royale":** Clears the screen and creates `goblin_task_start` task.
 * - **"GPIO_C":** Clears the screen and creates `GPIO_C_page_1` task.
 *
 * After launching a new task, the current `main_menu_task` deletes itself.
 * Display operations (e.g., `gpio_set_level(SS_display, 0/1)`, `clean_screen()`)
 * are managed to ensure smooth UI transitions.
 */
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
/**
 * @brief FreeRTOS task for the main page of the Notebook application.
 *
 * This task serves as the entry point for the Notebook app, presenting options
 * to create a new file, read existing files, edit files, or return to the main menu.
 *
 * @param pvParameters Unused.
 *
 * The task initializes by setting `coord_index_char` and calling `bootApp_noteBook()`
 * to draw the initial menu buttons. It then continuously monitors for touch input:
 * - **Watchdog Reset:** Resets the watchdog timer.
 * - **Touch Input:** Checks for touch coordinates.
 * - **Button Actions:**
 * - **"close" button:** Clears the screen and transitions to `main_menu_task`.
 * - **"New file" button:** Clears the screen, prompts for a new filename,
 * then creates and transitions to `keyboard_task` to handle input.
 * - **"Read files" button:** Clears the screen and transitions to `notebook_readfiles_task`.
 * - **Other (implicitly "Edit files") button:** Clears the screen and transitions to `notebook_editFilesPage1_task`.
 *
 * For all transitions, the screen is cleared, display SS line is managed, and the
 * current task deletes itself after creating the new one.
 */
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

					background_color = "black";
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
/**
 * @brief Adjusts the orientation of a goblin and the game map based on movement direction.
 *
 * This function determines if a goblin and the overall game map need their
 * horizontal orientation (left or right) flipped based on the provided movement
 * direction.
 *
 * @param direction A character representing the movement direction.
 * - 'd', 'e', 'c': Movement generally towards the right.
 * - 'q', 'a', 'w': Movement generally towards the left.
 * - Other: No horizontal movement, but checks for orientation inconsistencies.
 * @param goblin A pointer to a `goblin_torch` structure whose orientation and X-position
 * might be updated.
 *
 * The function works as follows:
 * - **Rightward Movement ('d', 'e', 'c'):**
 * - If the `goblin` is currently oriented 'l' (left), its `xp` (X-position) is
 * adjusted to mirror it across the screen's center (assuming a 480-pixel width
 * and a 20-pixel goblin width), and its `orientation` is set to 'r' (right).
 * - The `myMap.orientation` is set to 'r', and `set_orientation(1)` is called
 * to update the map's display orientation.
 * - **Leftward Movement ('q', 'a', 'w'):**
 * - If the `goblin` is currently oriented 'r' (right), its `xp` is adjusted
 * similarly for mirroring, and its `orientation` is set to 'l' (left).
 * - The `myMap.orientation` is set to 'l', and `set_orientation(5)` is called
 * to update the map's display orientation.
 * - **No Horizontal Movement (other directions):**
 * - This block handles cases where the `goblin` might have a different orientation
 * than the `myMap`. It ensures that the `myMap.orientation` is synchronized
 * with the `goblin->orientation`. If they differ, the `myMap.orientation`
 * is updated to match the `goblin`'s, and `set_orientation()` is called accordingly.
 */
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
/**
 * @brief Manages the execution of goblin tasks in a round-robin fashion.
 *
 * This function is responsible for cycling through a list of goblin tasks,
 * resuming one at a time, and suspending the current task if a "death"
 * condition is met.
 *
 * It performs the following actions:
 * - **Watchdog Reset:** Resets the ESP's watchdog timer to prevent system reboots.
 * - **Goblin Task Round-Robin:**
 * - Increments `current_goblin_index` and wraps it around using the modulo operator
 * (`% goblin_nb`) to ensure it stays within the bounds of `goblin_nb` (total number of goblins).
 * - Checks if the task handle at the `current_goblin_index` in the `goblinHandles` array
 * is not `NULL`.
 * - If a valid task handle is found, it calls `vTaskResume()` to resume that specific
 * goblin's task and then `break`s from the loop, effectively allowing only one
 * goblin task to be resumed per call to this function.
 * - **Death Condition Suspension:**
 * - If `death_bool` is true, the current task (presumably the one calling `step_func_handle_tasks`)
 * suspends itself indefinitely using `vTaskSuspend(NULL)`. This suggests that `death_bool`
 * signifies a game-over or critical state where further task execution for the current
 * context is not desired.
 */
static void step_func_handle_tasks() {

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
/**
 * @brief Animates a goblin's movement and updates its state.
 *
 * This function handles a single "step" of a goblin's movement, updating its
 * position, changing its orientation if needed, drawing animation frames,
 * and managing other tasks.
 *
 * @param direction The character representing the movement direction ('d', 'q', 's', 'z', 'e', 'a', 'w', 'c').
 * @param goblin A pointer to the `goblin_torch` structure representing the goblin.
 * @param enemy_id Unused parameter.
 *
 * The function performs the following actions:
 * - **Determines Movement Vector:** Based on the `direction` character, it sets `x1` and `y1`
 * for horizontal and vertical movement increments.
 * - **Updates Real Position:** Adjusts `goblin->real_xp` and `goblin->real_yp` by `x1 * 4` and `y1 * 4`
 * respectively, implying a finer-grained position tracking.
 * - **Orientation Handling:** For horizontal movements ('d', 'q', 'e', 'a', 'w', 'c'), it checks and
 * updates the goblin's `orientation` and mirrors its `xp` (display X-position) if the orientation changes.
 * It also updates `myMap.orientation` and calls `set_orientation()`. For vertical or no horizontal
 * movement, it ensures `myMap.orientation` matches `goblin->orientation`.
 * - **Boundary Check:** Before updating `goblin->xp` and `goblin->yp` (display positions), it checks
 * if the new position would be out of bounds (0-480 for X, 0-320 for Y). If it is, the function returns.
 * - **Animation Loop (5 frames):** The core of the function involves a repeated sequence for animation:
 * 1. **Update Display Position:** Increments `goblin->xp` and `goblin->yp` by `x1` and `y1`.
 * 2. **Task Management:** Calls `step_func_handle_tasks()` (presumably to resume other goblin tasks).
 * 3. **Settings Check:** Calls `step_func_settings_check()` to adjust orientations and map settings.
 * 4. **Clear Previous Frame:** Calls `clean_last_element_modified()` to erase the goblin's previous drawing.
 * 5. **Set Drawing Position:** Sets the display resolution position using `set_resolution_pos()`.
 * 6. **Set Pixel Format:** Sends display commands for pixel format (0x3A, 0x06).
 * 7. **Send Image Data:** Sends data for a specific animation frame (`goblin_torch_run0` through `goblin_torch_run3`).
 * 8. **Delay:** Introduces a small delay using `ets_delay_us(1)`.
 * - **Final Cleanup:** After the animation sequence, it decrements `goblin->xp` and `goblin->yp` by `x1` and `y1`
 * (this seems to undo the last increment, possibly to prepare for the *next* step's calculation) and
 * calls `clean_last_element_modified()` one last time.
 *
 * **Note:** The `delay` variable is always 1 microsecond, making its effect minimal. The repeated
 * code structure for each animation frame could potentially be refactored into a loop or helper function
 * for better maintainability.
 */
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
/**
 * @brief Animates the death sequence of a goblin.
 *
 * This function displays a series of animation frames to represent a goblin's
 * death, adjusting its display orientation and managing other tasks during the process.
 *
 * @param direction The initial direction ('l' for left, 'r' for right) the goblin was facing,
 * which influences the `step_func_settings_check` calls.
 * @param goblin A pointer to the `goblin_torch` structure representing the dying goblin.
 *
 * The function performs the following steps repeatedly for each of the 7 death animation frames
 * (`goblin_torch_run0`, `goblin_torch_death1` to `goblin_torch_death6`):
 * 1. **Adjust Orientation (if necessary):** If `direction` is 'r', it calls `step_func_settings_check`
 * with a 'd' (right) movement direction. If `direction` is 'l', it calls `step_func_settings_check`
 * with a 'q' (left) movement direction. This ensures the goblin and map orientation are consistent.
 * 2. **Set Display Position:** Uses `set_resolution_pos()` to prepare the display for drawing the goblin
 * at its current `xp` and `yp` coordinates, with a size of 20x20 pixels.
 * 3. **Set Pixel Format:** Sends display commands `0x3A` and `0x06` to set the interface pixel format.
 * 4. **Send Image Data:** Sends the pixel data for the current death animation frame (`goblin_torch_deathX[i]`)
 * using `send_ILI9488_data()`.
 * 5. **Delay:** Introduces a minimal delay of 1 microsecond using `ets_delay_us(1)`.
 * 6. **Handle Tasks:** Calls `step_func_handle_tasks()`, likely to allow other game tasks to run concurrently.
 *
 * Finally, after all animation frames are displayed, it updates `goblin->last_draw_coord_index` and
 * performs one last set of display position, pixel format, and image data sending for the final frame.
 *
 * **Note:** The `delay` variable being a constant 1 microsecond suggests that the animation
 * is designed to run as fast as possible on the hardware. The repeated code structure for
 * each frame could be refactored for conciseness using a loop and an array of image data pointers.
 */
static void goblin_torch_death(char direction, goblin_torch *goblin) {

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
/**
 * @brief Animates a goblin's attack sequence.
 *
 * This function renders a series of frames to depict a goblin's attack,
 * managing its orientation and allowing other game tasks to run concurrently.
 *
 * @param direction The initial direction ('l' for left, 'r' for right) the goblin was facing,
 * which influences `step_func_settings_check`.
 * @param goblin A pointer to the `goblin_torch` structure representing the attacking goblin.
 * @param enemy_id Unused parameter.
 *
 * The function iterates through several attack animation frames (from `goblin_torch_attack1`
 * to `goblin_torch_attack5` and back to `goblin_torch_run0`):
 * 1. **Adjust Orientation:** It calls `step_func_settings_check()` to ensure the goblin's
 * and map's orientation aligns with the attack direction.
 * 2. **Set Display Position:** `set_resolution_pos()` prepares the display for drawing.
 * 3. **Set Pixel Format:** Commands `0x3A` and `0x06` configure the display's pixel format.
 * 4. **Send Image Data:** The pixel data for the current animation frame is sent using
 * `send_ILI9488_data()`.
 * 5. **Delay:** A brief `ets_delay_us(1)` is introduced.
 * 6. **Handle Tasks:** `step_func_handle_tasks()` is called to allow other game elements to run.
 *
 * This sequence is repeated for each frame of the attack animation, culminating in the goblin
 * returning to its `goblin_torch_run0` (idle/running) state. The `clean_last_element_modified()`
 * is called at the end to clear the final attack frame from the screen.
 *
 * **Note:** The `delay` variable is fixed at 1 microsecond, suggesting a desire for fast animation.
 * The repetitive nature of the drawing and task handling could be streamlined with helper functions
 * or a more data-driven approach (e.g., an array of animation frames).
 */
static void goblin_torch_attack(char direction, goblin_torch *goblin, char enemy_id) {

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
/**
 * @brief Checks if two goblins are within each other's attack range.
 *
 * This function calculates the Manhattan distance (sum of absolute differences in X and Y coordinates)
 * between two goblins' real positions and compares it against a predefined `ATTACK_RANGE`.
 *
 * @param g1 A pointer to the first `goblin_torch` structure.
 * @param g2 A pointer to the second `goblin_torch` structure.
 * @return `true` if the goblins are within attack range, `false` otherwise.
 *
 * The function determines proximity by:
 * 1. Calculating the absolute difference in their `real_xp` (X-coordinates) as `dx`.
 * 2. Calculating the absolute difference in their `real_yp` (Y-coordinates) as `dy`.
 * 3. Returning `true` if both `dx` and `dy` are less than or equal to `ATTACK_RANGE`,
 * indicating they are close enough to attack.
 */
static bool in_attack_range(goblin_torch *g1, goblin_torch *g2) {

	int dx = abs(g1->real_xp - g2->real_xp);
	int dy = abs(g1->real_yp - g2->real_yp);

	return dx <= ATTACK_RANGE && dy <= ATTACK_RANGE;
}
/**
 * @brief Determines a character representing a cardinal or intercardinal movement direction
 * based on given X and Y displacements.
 *
 * This function takes horizontal (`dx`) and vertical (`dy`) displacement values
 * and returns a single character code indicating the corresponding movement direction.
 *
 * @param dx The displacement along the X-axis (horizontal).
 * - Positive `dx` indicates movement to the right.
 * - Negative `dx` indicates movement to the left.
 * - `dx` of 0 indicates no horizontal movement.
 * @param dy The displacement along the Y-axis (vertical).
 * - Positive `dy` indicates movement downwards (assuming a screen coordinate system
 * where Y increases downwards).
 * - Negative `dy` indicates movement upwards.
 * - `dy` of 0 indicates no vertical movement.
 * @return A character representing the movement direction:
 * - 'd': Right
 * - 'q': Left
 * - 'z': Up
 * - 's': Down
 * - 'a': Up-Left
 * - 'e': Up-Right
 * - 'c': Down-Right
 * - 'w': Down-Left
 * - 'n': No movement or undefined direction (if both `dx` and `dy` are 0).
 */

static char get_move_direction(int dx, int dy) {
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
/**
 * @brief Moves a goblin towards a specified enemy goblin.
 *
 * This function calculates the necessary displacement to move `self` (a goblin)
 * closer to `enemy` (another goblin), determines the appropriate movement direction,
 * and then initiates a step animation for `self` in that direction.
 *
 * @param self A pointer to the `goblin_torch` structure representing the goblin
 * that needs to move.
 * @param enemy A pointer to the `goblin_torch` structure representing the target enemy
 * towards which `self` should move.
 * @param enemy_id A character identifier for the enemy, passed through to `goblin_torch_step`.
 *
 * The function operates as follows:
 * 1. **Calculate Displacements:**
 * - It determines the horizontal difference (`dx`) between the enemy's real X-position
 * and the current goblin's real X-position.
 * - It determines the vertical difference (`dy`) between the enemy's real Y-position
 * and the current goblin's real Y-position.
 * 2. **Get Move Direction:** It calls `get_move_direction(dx, dy)` to translate these
 * displacements into a single character representing the cardinal or intercardinal
 * direction (e.g., 'd' for right, 'a' for up-left).
 * 3. **Execute Step:** It then calls `goblin_torch_step(dir, self, enemy_id)`, which
 * handles the actual movement, animation, and display updates for the `self` goblin
 * in the determined `dir`ection.
 */
static void move_towards_enemy(goblin_torch *self, goblin_torch *enemy, char enemy_id) {
	int dx = enemy->real_xp - self->real_xp;
	int dy = enemy->real_yp - self->real_yp;

	char dir = get_move_direction(dx, dy);

	goblin_torch_step(dir, self, enemy_id);
}

/**
 * @brief Finds the nearest living enemy goblin to a given goblin.
 *
 * This function iterates through all goblins in the `global_group`,
 * calculates the squared Euclidean distance to each one, and returns a
 * pointer to the closest living enemy.
 *
 * @param self A pointer to the `goblin_torch` structure representing the goblin
 * for whom the nearest enemy is being sought.
 * @return A pointer to the `goblin_torch` structure of the nearest living enemy,
 * or `NULL` if no other living goblins are found.
 *
 * The function operates as follows:
 * 1. **Initialization:** It sets `nearest` to `NULL` and `min_dist` to `INT_MAX`
 * (the largest possible integer value) to prepare for finding the minimum distance.
 * 2. **Iterate Through Goblins:** It loops through all goblins in the `global_group->goblins` array.
 * 3. **Skip Self and Dead Goblins:** For each `other` goblin, it checks two conditions:
 * - If `other` is the same as `self`, it skips to the next goblin (a goblin can't be its own enemy).
 * - If `other->health` is less than or equal to 0, it skips to the next goblin (dead goblins are not targets).
 * 4. **Calculate Squared Distance:**
 * - It calculates the horizontal (`dx`) and vertical (`dy`) differences between `self`'s
 * `real_xp`, `real_yp` and `other`'s `real_xp`, `real_yp`.
 * - It then calculates the **squared Euclidean distance** (`dist`) using the formula `dx*dx + dy*dy`.
 * Using squared distance avoids the computationally expensive `sqrt()` operation, which is
 * unnecessary for just comparing distances.
 * 5. **Update Nearest:** If the calculated `dist` is less than the current `min_dist`, it means
 * `other` is closer than any previously found enemy. In this case, `min_dist` is updated to `dist`,
 * and `nearest` is set to `other`.
 * 6. **Return Nearest:** After checking all goblins, the function returns the `nearest` goblin found.
 */

static goblin_torch* find_nearest_enemy(goblin_torch *self) {
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
/**
 * @brief Cleans up a goblin task after its "death" and manages task transitions.
 *
 * This function is designed to be called when a goblin's task needs to be terminated,
 * typically after the goblin has been defeated (its health reached zero). It handles
 * deleting the specific goblin's task, updating the global task handles,
 * and triggering the next game state or task.
 *
 * @param params Unused parameter, required by FreeRTOS task signature.
 *
 * The function performs the following actions:
 * 1. **Delete Goblin Task:** It deletes the FreeRTOS task associated with the
 * `current_goblin_index` from the `goblinHandles` array using `vTaskDelete()`.
 * This effectively removes the goblin's execution context from the FreeRTOS scheduler.
 * 2. **Clear Task Handle:** Sets the corresponding entry in `goblinHandles` to `NULL`
 * to indicate that this task slot is now free.
 * 3. **Handle Next Task:** Calls `step_func_handle_tasks()`, which likely
 * resumes the next active goblin task in the game's round-robin task management.
 * 4. **Signal Death:** Sets the global `death_bool` flag to `true`. This boolean
 * likely signifies a "game over" state or a critical event that should stop
 * the current thread of execution in the main game loop.
 * 5. **Delete Current Task:** Finally, it calls `vTaskDelete(NULL)` to delete
 * the `clean_task` itself. This is important because once cleanup is done and
 * the game state is updated, this temporary cleanup task is no longer needed.
 */
static void clean_task(void *params) {
	vTaskDelete(goblinHandles[current_goblin_index]);
	goblinHandles[current_goblin_index] = NULL;
	step_func_handle_tasks();
	death_bool = true;
	vTaskDelete(NULL);

}
/**
 * @brief FreeRTOS task for a single goblin's AI and actions in the Goblin Royale game.
 *
 * This task defines the behavior of an individual goblin, including finding enemies,
 * moving, attacking, and handling its own death.
 *
 * @param params A pointer to the `goblin_torch` structure representing this goblin instance.
 *
 * The task runs in an infinite loop, continuously performing the following actions:
 * 1. **Find Nearest Enemy:** It first identifies the `enemy` goblin closest to itself
 * using `find_nearest_enemy()`.
 * 2. **Behavioral Logic (if alive):**
 * - **Move Towards Enemy:** If the goblin's `health` is greater than 0 AND it's
 * `!in_attack_range()` of the `enemy`, it calls `move_towards_enemy()` to approach.
 * - **Attack Enemy:** If the goblin's `health` is greater than 0 AND it *is*
 * `in_attack_range()`, it initiates an attack animation via `goblin_torch_attack()`.
 * After the animation, it iterates through the `global_group` of goblins to find
 * the targeted `enemy` by `name` and reduces that enemy's `health` by 10, ensuring
 * health doesn't drop below zero.
 * 3. **Handle Death:** If the goblin's `health` is 0 or less, it executes its
 * death animation using `goblin_torch_death()`. It then sets `death_bool` to `false`
 * (potentially to indicate this specific goblin's death is being processed) and
 * creates a `clean_task` to handle its own task deletion, ensuring resources are freed.
 * 4. **Watchdog Reset:** At the end of each loop iteration, `esp_task_wdt_reset()`
 * is called to prevent the system's watchdog timer from timing out, which would
 * otherwise cause a system reboot.
 */
void goblin_task(void *params) {
	goblin_torch *goblin = (goblin_torch*) params;

	while (1) {

		// Find the nearest enemy that's alive
		goblin_torch *enemy = find_nearest_enemy(goblin);

		if (!in_attack_range(goblin, enemy) && goblin->health > 0) {

			move_towards_enemy(goblin, enemy, enemy->name[1]);
		} else if (goblin->health > 0) {

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

		esp_task_wdt_reset();

	}

}

