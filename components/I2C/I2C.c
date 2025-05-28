/**
 * @file I2C.c
 * @author your name (you@domain.com)
 * @brief this file contains an implementation of I2C protocol for ESP8266
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp8266/gpio_struct.h"
#include "driver/gpio.h"
#include "I2C.h"
#include "esp_task_wdt.h"

#define I2C_MY_ADR 0x42
#define I2C_DELAY_US 10  // Delay in microseconds
uint8_t buffer[6];

volatile bool SDA_falling = 0;
volatile bool SCL_rising = 0;
volatile bool SDA_rising = 0;

uint8_t I2C_SDA;
uint8_t I2C_SCL;
uint8_t I2C_SLAVE_ADDR;

/**
 * @brief Generates an I2C START condition.
 *
 * Initiates I2C communication by transitioning SDA from high to low
 * while SCL is high, followed by pulling SCL low to prepare for data transfer.
 * This function follows the I2C protocol's standard START sequence.
 */

void i2c_start() {
	gpio_set_level(I2C_SDA, 1);
	gpio_set_level(I2C_SCL, 1);
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SDA, 0);
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SCL, 0);
}

/**
 * @brief Generates an I2C STOP condition.
 *
 * Ends I2C communication by transitioning SDA from low to high
 * while SCL is high, signaling the release of the bus.
 * This function follows the I2C protocol's standard STOP sequence.
 */


void i2c_stop() {
	gpio_set_level(I2C_SDA, 0);
	gpio_set_level(I2C_SCL, 1);
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SDA, 1);
}

/**
 * @brief Generates a single I2C clock pulse on the SCL line.
 *
 * Toggles the SCL line high and then low with delays to simulate
 * one clock cycle in I2C communication. Useful for bit-level timing control.
 */


void one_tick() {
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SCL, 1);
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SCL, 0);
	ets_delay_us(I2C_DELAY_US);

}

/**
 * @brief Sends a single bit over the I2C bus.
 *
 * Sets the SDA line to the specified bit value, then generates one clock pulse
 * on the SCL line to transmit the bit.
 *
 * @param bit The bit value to send (true = 1, false = 0).
 */


static void i2c_send_bit(bool bit) {
	gpio_set_level(I2C_SDA, bit);
	one_tick();

}

/**
 * @brief Sends a byte over the I2C bus, MSB first.
 *
 * Transmits each bit of the given byte by calling i2c_send_bit from the most significant bit to the least significant bit.
 * After sending the byte, configures the SDA pin as input with pull-down enabled,
 * preparing to receive the ACK bit from the slave device.
 *
 * @param data The 8-bit data byte to send over I2C.
 */
void i2c_write_byte(uint8_t data) {
	for (int i = 7; i >= 0; i--) {
		i2c_send_bit((data >> i) & 1);

	}

	gpio_config_t io_conf_SDA_input = { .pin_bit_mask = (1ULL << I2C_SDA),
			.mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};

	gpio_config(&io_conf_SDA_input);
}

/**
 * @brief Reads the ACK/NACK bit from the I2C slave device.
 *
 * Releases the SDA line by configuring it as input, then
 * clocks the SCL line high to sample the ACK/NACK bit.
 * Prints "ACK = 1" if ACK (SDA low) is received, "ACK = 0" if NACK (SDA high).
 * Afterwards, configures SDA back as an output with pull-down enabled.
 */


static void recive_ACK_NACK() {

	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SCL, 1);
	ets_delay_us(I2C_DELAY_US);

	if (!gpio_get_level(I2C_SDA)) {
		printf("ACK = 1");
	} else {
		printf("ACK = 0");

	}
	gpio_set_level(I2C_SCL, 0);
	ets_delay_us(I2C_DELAY_US);

	gpio_config_t io_conf_SDA_output = { .pin_bit_mask = (1ULL << I2C_SDA),
			.mode = GPIO_MODE_OUTPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};

	gpio_config(&io_conf_SDA_output);
}

/**
 * @brief Sends a byte over I2C and reads the acknowledgment bit.
 *
 * Calls i2c_write_byte to transmit the byte, then reads the ACK/NACK
 * response from the slave device to confirm successful reception.
 *
 * @param byte The 8-bit data byte to send.
 */

void i2c_send_byte(uint8_t byte) {
	i2c_write_byte(byte);
	recive_ACK_NACK();
}

/**
 * @brief Reads a byte from the I2C bus, MSB first.
 *
 * Generates clock pulses on SCL to read each bit from the SDA line,
 * assembling them into an 8-bit byte.
 *
 * @return The byte read from the I2C slave device.
 */


static uint8_t i2c_read_byte() {
	bool bit;
	uint8_t recived = 0;
	for (int i = 7; i >= 0; i--) {
		ets_delay_us(I2C_DELAY_US);
		gpio_set_level(I2C_SCL, 1);
		ets_delay_us(I2C_DELAY_US);
		bit = gpio_get_level(I2C_SDA);
		recived |= bit << i;
		gpio_set_level(I2C_SCL, 0);
		ets_delay_us(I2C_DELAY_US);
	}
	return recived;
}

/**
 * @brief Sends an ACK or NACK bit on the I2C bus.
 *
 * Configures the SDA line as output, sets it to the specified ACK level,
 * and generates one clock pulse on the SCL line to transmit the bit.
 *
 * @param ACK If true, sends ACK (SDA low); if false, sends NACK (SDA high).
 */


void send_ACK_NACK(bool ACK) {

	gpio_config_t io_conf_SDA_output = { .pin_bit_mask = (1ULL << I2C_SDA),
			.mode = GPIO_MODE_OUTPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};

	gpio_config(&io_conf_SDA_output);
	gpio_set_level(I2C_SDA, ACK);
	one_tick();
}

/**
 * @brief Configures SDA as input and reads a byte from the I2C bus.
 *
 * Sets the SDA pin to input mode with pull-down enabled,
 * then calls i2c_read_byte to read an 8-bit byte from the bus.
 *
 * @return The byte received from the I2C slave device.
 */

uint8_t i2c_recive_byte() {
	gpio_config_t io_conf_SDA_input = { .pin_bit_mask = (1ULL << I2C_SDA),
			.mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};
	gpio_config(&io_conf_SDA_input);
	return i2c_read_byte();
}

/**
 * @brief FreeRTOS task that acts as an I2C master to send and receive data.
 *
 * This task continuously performs the following sequence:
 * - Sends a start condition.
 * - Sends the slave address with the write bit.
 * - Sends the string "Hello!" byte by byte.
 * - Sends a stop condition.
 * - Delays briefly.
 * - Sends a start condition.
 * - Sends the slave address with the read bit.
 * - Reads 6 bytes from the slave, sending ACK after each byte except the last one,
 *   which receives a NACK to signal the end of reading.
 * - Stores received bytes in a buffer, null-terminates it, and prints the result.
 * - Waits 1 second before repeating.
 *
 * @param pvParameters FreeRTOS task parameters (unused).
 */


static void i2c_task_master(void *pvParameters) {
	while (1) {
		i2c_start();
		i2c_send_byte(I2C_SLAVE_ADDR << 1);
		i2c_send_byte('H');
		i2c_send_byte('e');
		i2c_send_byte('l');
		i2c_send_byte('l');
		i2c_send_byte('o');
		i2c_send_byte('!');
		i2c_stop();

		ets_delay_us(I2C_DELAY_US);

		i2c_start();
		i2c_write_byte((I2C_SLAVE_ADDR << 1) | 1);
		one_tick();

		for (uint8_t j = 0; j < 6; j++) {
			uint8_t byte = i2c_recive_byte();
			if (j == 5)
				send_ACK_NACK(1);
			else
				send_ACK_NACK(0);
			// Ensure buffer does not overflow
			buffer[j] = byte;

		}

		buffer[6] = '\0';
		i2c_stop();

		printf("\n Received: %s\n", buffer);

		vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
	}
}

/**
 * @brief Initializes GPIO pins and starts the I2C master task.
 *
 * Configures the I2C SCL and SDA pins as outputs with appropriate pull-down
 * settings and no interrupts. Then creates and starts the FreeRTOS task
 * responsible for I2C master operations (`i2c_task_master`).
 */


void I2C_init_master() {
	gpio_config_t io_conf_SCL_output;
	io_conf_SCL_output.intr_type = GPIO_INTR_DISABLE;   // Disable interrupt
	io_conf_SCL_output.mode = GPIO_MODE_OUTPUT;        // Set as output mode
	io_conf_SCL_output.pin_bit_mask = (1ULL << I2C_SCL); // Set both SDA and SCL
	io_conf_SCL_output.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
	io_conf_SCL_output.pull_up_en = GPIO_PULLUP_DISABLE;  // Disable pull-up
	gpio_config(&io_conf_SCL_output);

	gpio_config_t io_conf_SDA_output = { .pin_bit_mask = (1ULL << I2C_SDA),
			.mode = GPIO_MODE_OUTPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};

	gpio_config(&io_conf_SDA_output);

	xTaskCreate(i2c_task_master, "i2c_task_master", 2048, NULL, 5, NULL);
}

/**
 * @brief Interrupt Service Routine (ISR) triggered on rising edge of SCL.
 *
 * Sets the `SCL_rising` flag to indicate that the SCL line has gone high.
 *
 * @param arg Pointer to ISR argument (unused).
 */

static IRAM_ATTR void SCL_rising_isr_handler(void *arg) {
	SCL_rising = 1;
}

/**
 * @brief Interrupt Service Routine (ISR) triggered on falling edge of SDA.
 *
 * Sets the `SDA_falling` flag to indicate that the SDA line has transitioned from high to low.
 *
 * @param arg Pointer to ISR argument (unused).
 */

static IRAM_ATTR void SDA_falling_isr_handler(void *arg) {
	SDA_falling = 1;
}

/**
 * @brief Detects an I2C start condition on the bus.
 * 
 * Monitors the I2C lines to detect the start condition, which occurs when
 * SDA falls while SCL is high. If detected, updates interrupt handlers:
 * removes ISR on SDA falling edge and adds ISR for SCL rising edge.
 * Resets the relevant flags accordingly.
 * 
 * @return true if a start condition is detected, false otherwise.
 */

static bool start_condition_detected() {
	while (gpio_get_level(I2C_SCL)) {
		if (SDA_falling) {
			SDA_falling = 0;
			gpio_isr_handler_remove(I2C_SDA);
			gpio_isr_handler_add(I2C_SCL, SCL_rising_isr_handler, NULL);
			SCL_rising = 0;
			return true;
		}
		esp_task_wdt_reset();
	}
	return false;
}

/**
 * @brief Receives a byte from the I2C master as a slave device.
 * 
 * Waits for the SCL rising edge interrupt to sample each bit on the SDA line,
 * assembling the bits into a byte. After receiving 8 bits, configures SDA as
 * an output and pulls it low to acknowledge receipt.
 * 
 * @return The byte received from the master.
 */

static uint8_t i2c_recive_byte_slave() {
	int i;
	uint8_t byte = 0;
	for (i = 7; i >= 0; i--) {
		while (!SCL_rising)
			;
		byte |= (gpio_get_level(I2C_SDA) << i);
		SCL_rising = 0;
	}

	gpio_config_t io_conf_SDA_output = { .pin_bit_mask = (1ULL << I2C_SDA),
			.mode = GPIO_MODE_OUTPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};

	gpio_config(&io_conf_SDA_output);
	gpio_set_level(I2C_SDA, 0);
	SCL_rising = 0;
	return byte;
}

/**
 * @brief Sends a byte to the I2C master as a slave device.
 * 
 * Configures SDA as output and transmits the given byte bit by bit,
 * synchronized with SCL rising edges detected via interrupt flags.
 * After sending all bits, reconfigures SDA as input to allow master to
 * send ACK/NACK.
 * 
 * @param byte The byte to send to the I2C master.
 */

static void i2c_send_byte_slave(uint8_t byte) {
	gpio_config_t io_conf_SDA_output = { .pin_bit_mask = (1ULL << I2C_SDA),
			.mode = GPIO_MODE_OUTPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};
	gpio_config_t io_conf_SDA_input = { .pin_bit_mask = (1ULL << I2C_SDA),
			.mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};

	gpio_config(&io_conf_SDA_output);
	int i;
	for (i = 7; i >= 0; i--) {
		gpio_set_level(I2C_SDA, byte >> i);
		while (!SCL_rising)
			;
		SCL_rising = 0;
	}
	gpio_config(&io_conf_SDA_input);
	SCL_rising = 0;
}

/**
 * @brief Detects an I2C stop condition on the bus.
 * 
 * Monitors the SDA line while the SCL line is high to detect a rising edge on SDA,
 * which signals a stop condition in I2C communication. If detected, it reconfigures
 * the SDA pin as input, updates ISR handlers accordingly, and returns true.
 * 
 * @return true if a stop condition is detected, false otherwise.
 */

static bool stop_condition_detected() {
	while (gpio_get_level(I2C_SCL)) {
		if (SDA_rising) {
			gpio_config_t io_conf_SDA_input = { .pin_bit_mask =
					(1ULL << I2C_SDA), .mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
					.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
							GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
					};
			gpio_config(&io_conf_SDA_input);
			gpio_isr_handler_add(I2C_SDA, SDA_falling_isr_handler, NULL);
			gpio_isr_handler_remove(I2C_SCL);
			return true;
		}
	}
	gpio_isr_handler_remove(I2C_SDA);
	return false;
}

/**
 * @brief Waits for SCL rising edge and releases SDA line for ACK/NACK from master.
 * 
 * This function blocks until the SCL line goes high (rising edge), then resets the
 * SCL rising edge flag and configures the SDA pin as an input with a pull-down resistor.
 * This allows the master device to send an ACK or NACK bit during I2C communication.
 */

static void send_ACK_NACK_slave() {
	while (!SCL_rising)
		;
	SCL_rising = 0;
	gpio_config_t io_conf_SDA_input = { .pin_bit_mask = (1ULL << I2C_SDA),
			.mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
			.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
					GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
			};
	gpio_config(&io_conf_SDA_input);
}

/**
 * @brief Waits for SCL rising edge and reads ACK/NACK bit from the master on SDA line.
 * 
 * This function blocks until the SCL line goes high (rising edge), then reads the
 * level of the SDA line, which represents the ACK (0) or NACK (1) bit sent by the master.
 * After reading, it clears the SCL rising edge flag and returns the bit.
 * 
 * @return true if NACK (SDA high), false if ACK (SDA low)
 */

static bool read_ACK_NACK_slave() {
	while (!SCL_rising)
		;
	bool ack = gpio_get_level(I2C_SDA);
	SCL_rising = 0;
	return ack;
}

/**
 * @brief I2C slave task that waits for start conditions, receives bytes,
 *        sends ACK/NACK, and processes the slave address and R/W bit.
 * 
 * Continuously monitors the bus for start conditions, reads one byte from
 * the master, sends an acknowledgment, and prints the received byte.
 * Checks if the byte matches the slave address and handles read/write requests.
 * 
 * @param pvParameters Task parameter (unused).
 */

static void i2c_task_slave(void *pvParameters) {
	while (1) {
		SDA_falling = 0;
		bool start = start_condition_detected();
		if (start) {
			uint8_t byte = i2c_recive_byte_slave();
			send_ACK_NACK_slave();
			printf("Byte in hex: 0x%02X\n", byte);
			if ((byte >> 1) == I2C_MY_ADR) {
				if ((byte & 1) == 1) {

					// handle salve sending after you buy a logic analyser or an oscilloscope

				}
			} else {
				// handle salve receiving after you buy a logic analyser or an oscilloscope
			}
		}
	}
	esp_task_wdt_reset();
}

void I2C_init_salve() {

	//gpio_config_t io_conf_SCL_input;
	//io_conf_SCL_input.mode = GPIO_MODE_INPUT;          // Set as output mode
	//io_conf_SCL_input.pin_bit_mask = (1ULL << I2C_SCL); // Set both SDA and SCL
	//io_conf_SCL_input.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
	//io_conf_SCL_input.pull_up_en = GPIO_PULLUP_DISABLE;
	//io_conf_SCL_input.intr_type = GPIO_INTR_POSEDGE;
	//
	//gpio_config(&io_conf_SCL_input);
	//
	//io_conf_SDA_input.intr_type = GPIO_INTR_NEGEDGE;
	//io_conf_SDA_input.pull_down_en = GPIO_PULLDOWN_ENABLE;
	//
	//gpio_config(&io_conf_SDA_input);
	//
	//gpio_install_isr_service(0);
	//
	//gpio_isr_handler_add(I2C_SDA, SDA_falling_isr_handler, NULL);
	//
	//gpio_set_level(I2C_SCL, 1);
	//gpio_set_level(I2C_SDA, 1);
	//
	//xTaskCreate(i2c_task_slave, "i2c_task_slave", 6048, NULL,
	//configMAX_PRIORITIES - 1, NULL);
}
