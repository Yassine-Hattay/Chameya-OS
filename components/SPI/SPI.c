/**
 * @file SPI.c
 * @author your name (you@domain.com)
 * @brief this file implements SPI communication in bit-bang mode for ESP8266.
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "SPI.h"

volatile bool SS_level = 1;

uint8_t BMOSI;
uint8_t BSS;
uint8_t BSCK;

/**
 * @brief Performs a full-duplex SPI communication in bit-bang mode 0.
 *
 * This function simulates SPI Master communication using bit-banging for mode 0.
 * It sends 8 bits of `data_to_send` and simultaneously receives data from the MISO line.
 * The clock (BSCK) is set low, and Chip Select (BSS) is pulled low to initiate communication.
 * Data is sent bit by bit on the MOSI line, clocked by toggling BSCK.
 *
 * @param data_to_send The 8-bit data to be sent over MOSI.
 * @return The 8-bit data received from the MISO line (currently commented out and returns 0).
 */

 uint8_t spi_master_bit_bang_mode_0(uint8_t data_to_send) {
	uint8_t received = 0;
	gpio_set_level(BSCK, 0);
	// Pull CS low to start communication
	gpio_set_level(BSS, 0);
	ets_delay_us(10);

	for (int i = 7; i >= 0; i--) {
		// Set MOSI
		gpio_set_level(BMOSI, (data_to_send >> i) & 1);

		ets_delay_us(10);

		gpio_set_level(BSCK, 1);
		ets_delay_us(10);

		received |= (gpio_get_level(MISO) << i);
		gpio_set_level(BSCK, 0);
	}

	// Return received data
	return received;
}

/**
 * @brief Performs a full-duplex SPI communication in bit-bang mode 1.
 *
 * This function simulates SPI Master communication using bit-banging for mode 1.
 * It sends 8 bits of `data_to_send` and simultaneously receives data from the MISO line.
 * The clock (SCK) is initially low, and Chip Select (SS) is pulled low to begin communication.
 * For each bit, MOSI is set, SCK goes high and then low, and data is read from MISO.
 * Chip Select (SS) is pulled high to end the communication.
 *
 * @param data_to_send The 8-bit data to be sent over MOSI.
 * @return The 8-bit data received from the MISO line.
 */

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

/**
 * @brief Performs a full-duplex SPI communication in bit-bang mode 2.
 *
 * This function simulates SPI Master communication using bit-banging for mode 2.
 * It sends 8 bits of `data_to_send` and simultaneously receives data from the MISO line.
 * The clock (SCK) is initially high, and Chip Select (SS) is pulled low to begin communication.
 * For each bit, MOSI is set, SCK goes low and then high, and data is read from MISO.
 * Chip Select (SS) is pulled high to end the communication.
 *
 * @param data_to_send The 8-bit data to be sent over MOSI.
 * @return The 8-bit data received from the MISO line.
 */

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

/**
 * @brief Performs a full-duplex SPI communication in bit-bang mode 3.
 *
 * This function simulates SPI Master communication using bit-banging for mode 3.
 * It sends 8 bits of `data_to_send` and simultaneously receives data from the MISO line.
 * The clock (SCK) is initially high, and Chip Select (SS) is pulled low to begin communication.
 * For each bit, MOSI is set, SCK goes low and then high, and data is read from MISO.
 * Chip Select (SS) is pulled high to end the communication.
 *
 * @param data_to_send The 8-bit data to be sent over MOSI.
 * @return The 8-bit data received from the MISO line.
 */
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
/**
 * @brief FreeRTOS task for continuous SPI master communication in bit-bang mode 3.
 *
 * This task continuously sends a fixed data byte (0xAA) using `spi_master_bit_bang_mode_3`
 * and prints the received data to the console. It then delays for 1 second before
 * repeating the operation. This function is intended to be run as a FreeRTOS task.
 *
 * @param pvParameter Unused parameter, required by FreeRTOS task signature.
 */
static void spi_master_task(void *pvParameter) {
	uint8_t data_to_send = 0xAA;  // Example data
	uint8_t received_data = 0;

	while (1) {
		received_data = spi_master_bit_bang_mode_3(data_to_send);
		printf("Received: 0x%02X\n", received_data);

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
/**
 * @brief Initializes the GPIO pins for SPI master bit-bang communication.
 *
 * This function configures the MOSI, SCK, and SS pins as outputs and the MISO pin as an input.
 * All pull-up and pull-down resistors are disabled, and interrupts are disabled for these pins.
 * It also sets the initial level of the MOSI pin to low and the SS (Chip Select) pin to high,
 * ensuring the slave is not selected at initialization.
 */
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

	gpio_set_level(MOSI, 0);
	gpio_set_level(SS,1);

}

void IRAM_ATTR ss_isr_handler(void *arg) {
	SS_level = 0;
}
/**
 * @brief Performs SPI slave communication in bit-bang mode 2.
 *
 * This function acts as an SPI slave, waiting for a master to initiate communication.
 * It operates in SPI Mode 2, where the clock is high when idle, and data is sampled
 * on the falling edge and changed on the rising edge.
 * The slave actively waits for clock edges to receive data on MOSI and simultaneously
 * sends `response_data` on MISO.
 *
 * @param response_data The 8-bit data to be sent back to the master.
 */
static void spi_slave_bit_bang_mode_2(uint8_t response_data) {
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
/**
 * @brief Performs SPI slave communication in bit-bang mode 3.
 *
 * This function acts as an SPI slave, waiting for a master to initiate communication.
 * It operates in SPI Mode 3, where the clock is high when idle, and data is sampled
 * on the rising edge and changed on the falling edge.
 * The slave actively waits for clock edges to receive data on MOSI and simultaneously
 * sends `response_data` on MISO.
 *
 * @param response_data The 8-bit data to be sent back to the master.
 */
static void spi_slave_bit_bang_mode_3(uint8_t response_data) {
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

static void spi_slave_bit_bang_mode_1(uint8_t response_data) {
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
/**
 * @brief Performs SPI slave communication in bit-bang mode 1.
 *
 * This function acts as an SPI slave, waiting for a master to initiate communication.
 * It operates in SPI Mode 1, where the clock is low when idle, and data is sampled
 * on the rising edge and changed on the falling edge.
 * The slave actively waits for clock edges to receive data on MOSI and simultaneously
 * sends `response_data` on MISO.
 *
 * @param response_data The 8-bit data to be sent back to the master.
 */
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
/**
 * @brief FreeRTOS task for continuous SPI slave communication in bit-bang mode 0.
 *
 * This task continuously acts as an SPI slave, responding with a fixed data byte (0x43)
 * using `spi_slave_bit_bang_mode_0`. It also periodically resets the
 * watchdog timer to prevent the task from being reset. This function is intended
 * to be run as a FreeRTOS task.
 *
 * @param pvParameter Unused parameter, required by FreeRTOS task signature.
 */
static void spi_slave_task(void *pvParameter) {
	uint8_t response_data = 0x43;
	while (1) {

		spi_slave_bit_bang_mode_0(response_data);

		esp_task_wdt_reset(); // Keep the watchdog timer from resetting the task

	}
}


/**
 * @brief Initializes the GPIO pins and sets up the necessary components for SPI slave bit-bang communication.
 *
 * This function configures the MISO pin as an output. It then configures the
 * SS (Slave Select) pin as an input with a pull-up resistor enabled and
 * registers an interrupt service routine (ISR) to trigger on its falling edge,
 * indicating a master's selection. The SCK (clock) and MOSI (Master Out Slave In)
 * pins are configured as inputs without pull-up/down resistors.
 * Finally, it sets the initial state of MISO to low and creates a FreeRTOS task
 * to handle the actual SPI slave communication.
 */

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

	gpio_config_t io_conf_SCK_MOSI = { .pin_bit_mask = (1ULL << SCK)
			| (1ULL << MOSI), .mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
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
