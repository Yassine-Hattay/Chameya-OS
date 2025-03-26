#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"

#define MOSI 13
#define MISO 12
#define SCK  14
#define SS   15

volatile bool SS_level = 1;

// Full-duplex SPI: Send and receive at the same time
uint8_t spi_master_bit_bang_mode_0(uint8_t data_to_send) {
	uint8_t received = 0;
	gpio_set_level(SCK, 0);
	// Pull CS low to start communication
	gpio_set_level(SS, 0);
	ets_delay_us(10);

	for (int i = 7; i >= 0; i--) {
		// Set MOSI
		gpio_set_level(MOSI, (data_to_send >> i) & 1);
		ets_delay_us(10);

		gpio_set_level(SCK, 1);
		ets_delay_us(10);

		received |= (gpio_get_level(MISO) << i);
		gpio_set_level(SCK, 0);

	}

	// Pull CS high to end communication
	gpio_set_level(SS, 1);

	// Return received data
	return received;
}

// Full-duplex SPI: Send and receive at the same time
uint8_t spi_master_bit_bang_mode_1(uint8_t data_to_send) {
	uint8_t received = 0;
	gpio_set_level(SCK, 0);
	// Pull CS low to start communication
	gpio_set_level(SS, 0);
	ets_delay_us(10);

	for (int i = 7; i >= 0; i--) {
		// Set MOSI
		gpio_set_level(MOSI, (data_to_send >> i) & 1);
		gpio_set_level(SCK, 1);
		ets_delay_us(10);
		gpio_set_level(SCK, 0);
		ets_delay_us(10);
		received |= (gpio_get_level(MISO) << i);

	}

	// Pull CS high to end communication
	gpio_set_level(SS, 1);

	// Return received data
	return received;
}

// Full-duplex SPI: Send and receive at the same time
uint8_t spi_master_bit_bang_mode_2(uint8_t data_to_send) {
	uint8_t received = 0;
	gpio_set_level(SCK, 1);
	// Pull CS low to start communication
	gpio_set_level(SS, 0);
	ets_delay_us(10);

	for (int i = 7; i >= 0; i--) {
		// Set MOSI
		gpio_set_level(MOSI, (data_to_send >> i) & 1);
		ets_delay_us(10);
		gpio_set_level(SCK, 0);
		ets_delay_us(10);
		received |= (gpio_get_level(MISO) << i);
		gpio_set_level(SCK, 1);

	}

	// Pull CS high to end communication
	gpio_set_level(SS, 1);

	// Return received data
	return received;
}

// Full-duplex SPI: Send and receive at the same time
uint8_t spi_master_bit_bang_mode_3(uint8_t data_to_send) {
	uint8_t received = 0;
	gpio_set_level(SCK, 1);
	// Pull CS low to start communication
	gpio_set_level(SS, 0);
	ets_delay_us(10);

	for (int i = 7; i >= 0; i--) {
		// Set MOSI
		gpio_set_level(MOSI, (data_to_send >> i) & 1);
		gpio_set_level(SCK, 0);
		ets_delay_us(10);
		gpio_set_level(SCK, 1);
		ets_delay_us(10);
		received |= (gpio_get_level(MISO) << i);

	}

	// Pull CS high to end communication
	gpio_set_level(SS, 1);

	// Return received data
	return received;
}

void spi_master_task(void *pvParameter) {
	uint8_t data_to_send = 0xAA;  // Example data
	uint8_t received_data = 0;

	while (1) {
		received_data = spi_master_bit_bang_mode_3(data_to_send);
		printf("Received: 0x%02X\n", received_data);

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void spi_master_init(void) {
	// Configure GPIO
	gpio_config_t io_conf = { .pin_bit_mask = (1ULL << MOSI) | (1ULL << SCK)
			| (1ULL << SS), .mode = GPIO_MODE_OUTPUT, .pull_up_en =
			GPIO_PULLUP_DISABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type = GPIO_INTR_DISABLE };
	gpio_config(&io_conf);

	// Configure MISO pin as input
	gpio_config_t io_conf_slave = { .pin_bit_mask = (1ULL << MISO), .mode =
			GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
			GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE };
	gpio_config(&io_conf_slave);
	// Default pin states

	gpio_set_level(MOSI, 0);
	gpio_set_level(SS, 1);

	//xTaskCreate(spi_task, "spi_task", 1024, NULL, 1, NULL);
	xTaskCreate(spi_master_task, "spi_master_task", 1024, NULL, 1, NULL);

}

void IRAM_ATTR ss_isr_handler(void *arg) {
	SS_level = 0;
}

void spi_slave_bit_bang_mode_2(uint8_t response_data) {
	uint8_t received_data = 0;

	if (!SS_level) {
		for (int i = 7; i >= 0; i--) {
			while (gpio_get_level(SCK) == 1)
				;
			while (gpio_get_level(SCK) == 0)
				;
			received_data |= (gpio_get_level(MOSI) << i);
			gpio_set_level(MISO, (response_data >> i) & 1);

		}
		printf("Received: 0x%02X | Sent: 0x%02X\n", received_data,
						response_data);
		SS_level = 1;
	}
}

void spi_slave_bit_bang_mode_3(uint8_t response_data) {
	uint8_t received_data = 0;

	if (!SS_level) {
		for (int i = 7; i >= 0; i--) {
			while (gpio_get_level(SCK) == 1)
				;
			gpio_set_level(MISO, (response_data >> i) & 1);
			while (gpio_get_level(SCK) == 0)
				;
			received_data |= (gpio_get_level(MOSI) << i);

		}
		printf("Received: 0x%02X | Sent: 0x%02X\n", received_data,
						response_data);
		SS_level = 1;
	}
}

void spi_slave_bit_bang_mode_1(uint8_t response_data) {
	uint8_t received_data = 0;

	if (!SS_level) {
		for (int i = 7; i >= 0; i--) {
			while (gpio_get_level(SCK) == 0)
				;
			gpio_set_level(MISO, (response_data >> i) & 1);
			while (gpio_get_level(SCK) == 1)
				;
			received_data |= (gpio_get_level(MOSI) << i);

		}
		printf("Received: 0x%02X | Sent: 0x%02X\n", received_data,
						response_data);
		SS_level = 1;
	}
}

void spi_slave_bit_bang_mode_0(uint8_t response_data) {
	uint8_t received_data = 0;

	if (!SS_level) {
		for (int i = 7; i >= 0; i--) {
			while (gpio_get_level(SCK) == 1)
				;
			received_data |= (gpio_get_level(MOSI) << i);
			while (gpio_get_level(SCK) == 0)
				;
			gpio_set_level(MISO, (response_data >> i) & 1);

		}
		printf("Received: 0x%02X | Sent: 0x%02X\n", received_data,
						response_data);
		SS_level = 1;
	}
}

void spi_slave_task(void *pvParameter) {
	uint8_t response_data = 0x43;
	while (1) {

		spi_slave_bit_bang_mode_0(response_data);

		esp_task_wdt_reset(); // Keep the watchdog timer from resetting the task

	}
}
void spi_slave_init(void) {
// Configure GPIO for MISO (Output)
	gpio_config_t io_conf_MISO = { .pin_bit_mask = (1ULL << MISO), .mode =
			GPIO_MODE_OUTPUT,  // MISO as output
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE };
	gpio_config(&io_conf_MISO);

// Configure GPIO for MOSI, SCK, SS (Input with Interrupt)
	gpio_config_t io_conf_SS = { .pin_bit_mask = (1ULL << SS), .mode =
			GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_ENABLE, .pull_down_en =
					GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_NEGEDGE // Interrupt on falling edge
			};
	gpio_config(&io_conf_SS);

	gpio_config_t io_conf_SCK_MOSI = { .pin_bit_mask = (1ULL << SCK) | (1ULL
			<< MOSI), .mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};

	gpio_config(&io_conf_SCK_MOSI);

	// Set default MISO state
	gpio_set_level(MISO, 0);

	gpio_install_isr_service(0);
	// Install ISR handlers
	gpio_isr_handler_add(SS, ss_isr_handler, NULL);    // SS interrupt handler

// Create SPI slave task
	xTaskCreate(spi_slave_task, "spi_slave_task", 1024, NULL, 1, NULL);
}
