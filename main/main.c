#include "../components/UART/UART.h"
#include "../components/ILI9488_driver/ILI9488_driver.h"

#include "esp_system.h"

void transform_array(const uint8_t *input, uint8_t *output, int input_size,
		int font_size) {
	size_t output_idx = 0;
	for (size_t i = 0; i < input_size; i++) {

		if (input[i] == 0x00) {
			output[output_idx++] = 0x00;
			output[output_idx++] = 0x00;
		} else if (input[i] == 0xFF) {
			output[output_idx++] = 0xFF;
			output[output_idx++] = 0xFF;
		} else if (input[i] == 0xF8) {
			output[output_idx++] = 0xFF;
			output[output_idx++] = 0x00;
		} else if (input[i] == 0x07) {
			output[output_idx++] = 0x00;
			output[output_idx++] = 0xFF;
		}

		if ((i + 1) % (3 * font_size) == 0) {
			int j = 0;

			for (int index = output_idx - (3 * 2 * font_size);
					index <= output_idx - 1; index++) {
				output[output_idx + j] = output[index];
				j = j + 1;
			}
			output_idx = output_idx + 6 * font_size;
		}

	}
}

void app_main() {
	esp_set_cpu_freq(ESP_CPU_FREQ_160M);  // Set CPU speed to 160 MHz
	uart_t uart0 = { 0, 3, 1, 1, 1, 115200 }; // UART0 (TX: GPIO1, RX: GPIO3) @ 115200 baud

	my_uart_init(&uart0);

	size_t max_output_size = dataSize * 4;

	size_t max_output_size1 = max_output_size * 4;

	size_t max_output_size2 = max_output_size1 * 4;

	size_t max_output_size3 = max_output_size2 * 4;

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

	transform_array(A_font_0, output, dataSize, 1);

	transform_array(output, output1, max_output_size, 2);
	free(output);

	transform_array(output1, output2, max_output_size1, 4);
	free(output1);

	transform_array(output2, output3, max_output_size2, 8);

	free(output2);

	init_display();

	set_orientation(5);

	set_resolution_pos(240, 110, 96, 112);

	printf("writing to frame memory ! \n");

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x01);

	send_command(0x2C);

	for (uint64_t i = 0; i < max_output_size3; i++) {
		send_ILI9488_data(output3[i]);
	}

	for (uint64_t i = 0; i < max_output_size2; i++) {
		printf("%02X ", output2[i]);
		if ((i + 1) % 6 == 0) {
			printf("\n");
		}

	}

}

