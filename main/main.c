#include "../components/UART/UART.h"
#include "../components/I2C/I2C.h"
#include "../components/SPI/SPI.h"
#include "unity.h"

#include "esp_system.h"

uint8_t data_to_send = 0xAA;  // Example data
uint8_t received_data = 0;

void app_main() {
	esp_set_cpu_freq(ESP_CPU_FREQ_160M);  // Set CPU speed to 160 MHz

	spi_master_init();

	while (1) {
		received_data = spi_master_bit_bang_mode_3(0xAA);
		printf("Received: 0x%02X\n", received_data);

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

