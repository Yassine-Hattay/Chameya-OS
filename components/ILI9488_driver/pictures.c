/*
 * assets.c
 *
 *  Created on: 5 Apr 2025
 *      Author: hatta
 */

#include "ILI9488_driver.h"

const uint8_t A_font_0[] ICACHE_RODATA_ATTR = {
0x07,0xFF,0x00,
0xF8,0x00,0xF8,
0xF8,0x00,0xF8,
0xFF,0xFF,0xF8,
0xF8,0x00,0xF8,
0xF8,0x00,0xF8,
0xF8,0x00,0xF8
 };

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

const size_t dataSize = sizeof(A_font_0);

