#include "../components/UART/UART.h"
#include "../components/ILI9488_driver/ILI9488_driver.h"

#include "esp_system.h"

void printf_ILI9488(const char *message, uint16_t x, uint16_t y,
		uint8_t font_size) {
	uint16_t width;
	uint16_t height;

	switch (font_size) {
	case 0:
		width = 6;
		height = 7;
		break;
	case 1:
		width = 12;
		height = 14;
		break;
	case 2:
		width = 24;
		height = 28;
		break;
	case 3:
		width = 48;
		height = 56;
		break;
	case 4: // same values for 3 and 4
		width = 96;
		height = 112;
		break;

	case 5: // same values for 3 and 4
		width = 192;
		height = 224;
		break;
	default:
		printf("error select a font size between 0 and 5 ! \n");
		return;

	}

	while (*message) {
		if (*message == 'a') {
			letter_font_0_size = sizeof(A_font_0);

			set_resolution_pos(x + somthing, y, width, height);

			send_command(0x2C);

			for (uint64_t i = 0; i < letter_font_0_size; i++) {
				send_ILI9488_data(A_font_0[i]);
			}

			message++;
		}

	}

}

void app_main() {
	esp_set_cpu_freq(ESP_CPU_FREQ_160M);  // Set CPU speed to 160 MHz
	uart_t uart0 = { 0, 3, 1, 1, 1, 115200 }; // UART0 (TX: GPIO1, RX: GPIO3) @ 115200 baud

	my_uart_init(&uart0);

	size_t max_output_size = letter_font_0_size * 4;

	size_t max_output_size1 = max_output_size * 4;

	size_t max_output_size2 = max_output_size1 * 4;

	size_t max_output_size3 = max_output_size2 * 4;

	size_t max_output_size4 = max_output_size3 * 4;

	// Allocate output buffer
	uint8_t *output = (uint8_t*) malloc(max_output_size);
	if (!output) {
		printf("Memory allocation failed.\n");
	}

	uint8_t *output1 = (uint8_t*) malloc(max_output_size1);
	if (!output1) {
		printf("Memory allocation failed.\n");
	}

	uint8_t *output2 = (uint8_t*) malloc(max_output_size2);
	if (!output2) {
		printf("Memory allocation failed.\n");
	}

	uint8_t *output3 = (uint8_t*) malloc(max_output_size3);
	if (!output3) {
		printf("Memory allocation failed.\n");
	}

	uint8_t *output4 = (uint8_t*) malloc(max_output_size4);
	if (!output3) {
		printf("Memory allocation failed.\n");
	}

	transform_array(devide_font_0, output, letter_font_0_size, 1);

	transform_array(output, output1, max_output_size, 2);
	free(output);

	transform_array(output1, output2, max_output_size1, 4);
	free(output1);

	transform_array(output2, output3, max_output_size2, 8);

	free(output2);

	transform_array(output3, output4, max_output_size3, 16);

	free(output3);

	printf_ILI9488();

	init_display();

	set_orientation(1);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x01);

}

