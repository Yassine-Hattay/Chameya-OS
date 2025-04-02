#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp8266/gpio_struct.h"
#include "driver/gpio.h"
#include "I2C.h"
#include "esp_task_wdt.h"

#define I2C_SDA 4  // GPIO for SDA
#define I2C_SCL 5  // GPIO for SCL
#define I2C_SLAVE_ADDR 0x42  // ESP32 Slave Address
#define I2C_MY_ADR 0x42
#define I2C_DELAY_US 10  // Delay in microseconds
uint8_t buffer[BUFFER_SIZE];

volatile bool SDA_falling = 0;
volatile bool SCL_rising = 0;
volatile bool SDA_rising = 0;

gpio_config_t io_conf_SDA_input = { .pin_bit_mask = (1ULL << I2C_SDA),
		.mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
		.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en = GPIO_PULLDOWN_ENABLE,
		.intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
		};

gpio_config_t io_conf_SDA_output = { .pin_bit_mask = (1ULL << I2C_SDA),
		.mode = GPIO_MODE_OUTPUT, // MOSI, SCK, and SS as input
		.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en = GPIO_PULLDOWN_ENABLE,
		.intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
		};

void i2c_start() {
	gpio_set_level(I2C_SDA, 1);
	gpio_set_level(I2C_SCL, 1);
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SDA, 0);
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SCL, 0);
}

void i2c_stop() {
	gpio_set_level(I2C_SDA, 0);
	gpio_set_level(I2C_SCL, 1);
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SDA, 1);
}

void one_tick() {
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SCL, 1);
	ets_delay_us(I2C_DELAY_US);
	gpio_set_level(I2C_SCL, 0);
	ets_delay_us(I2C_DELAY_US);

}

void i2c_send_bit(bool bit) {
	gpio_set_level(I2C_SDA, bit);
	one_tick();

}

void i2c_write_byte(uint8_t data) {
	for (int i = 7; i >= 0; i--) {
		i2c_send_bit((data >> i) & 1);

	}

	gpio_config(&io_conf_SDA_input);
}

void recive_ACK_NACK() {

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

	gpio_config(&io_conf_SDA_output);
}

void i2c_send_byte(uint8_t byte) {
	i2c_write_byte(byte);
	recive_ACK_NACK();
}

uint8_t i2c_read_byte() {
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

void send_ACK_NACK(bool ACK) {
	gpio_config(&io_conf_SDA_output);
	gpio_set_level(I2C_SDA, ACK);
	one_tick();
}

uint8_t i2c_recive_byte() {
	gpio_config(&io_conf_SDA_input);
	return i2c_read_byte();
}

void i2c_task_master(void *pvParameters) {
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

void I2C_init_master() {
	gpio_config_t io_conf_SCL_output;
	io_conf_SCL_output.intr_type = GPIO_INTR_DISABLE;   // Disable interrupt
	io_conf_SCL_output.mode = GPIO_MODE_OUTPUT;        // Set as output mode
	io_conf_SCL_output.pin_bit_mask = (1ULL << I2C_SCL); // Set both SDA and SCL
	io_conf_SCL_output.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
	io_conf_SCL_output.pull_up_en = GPIO_PULLUP_DISABLE;  // Disable pull-up
	gpio_config(&io_conf_SCL_output);                // Apply configurationé

	gpio_config(&io_conf_SDA_output);                 // Apply configuration

	xTaskCreate(i2c_task_master, "i2c_task_master", 2048, NULL, 5, NULL);
}

static IRAM_ATTR void SCL_rising_isr_handler(void *arg) {
	SCL_rising = 1;
}

static IRAM_ATTR void SDA_falling_isr_handler(void *arg) {
	SDA_falling = 1;
}

static IRAM_ATTR void SDA_rising_isr_handler(void *arg) {
	SDA_rising = 1;
}

bool start_condition_detected() {
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

uint8_t i2c_recive_byte_slave() {
	int i;
	uint8_t byte = 0;
	for (i = 7; i >= 0; i--) {
		while (!SCL_rising)
			;
		byte |= (gpio_get_level(I2C_SDA) << i);
		SCL_rising = 0;
	}
	gpio_config(&io_conf_SDA_output);
	gpio_set_level(I2C_SDA, 0);
	SCL_rising = 0;
	return byte;
}

void i2c_send_byte_slave(uint8_t byte) {
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

bool stop_condition_detected() {
	while (gpio_get_level(I2C_SCL)) {
		if (SDA_rising) {
			io_conf_SDA_input.intr_type = GPIO_INTR_NEGEDGE;
			gpio_config(&io_conf_SDA_input);
			gpio_isr_handler_add(I2C_SDA, SDA_falling_isr_handler, NULL);
			gpio_isr_handler_remove(I2C_SCL);
			return true;
		}
	}
	gpio_isr_handler_remove(I2C_SDA);
	return false;
}

void send_ACK_NACK_slave() {
	while (!SCL_rising)
		;
	SCL_rising = 0;

	gpio_config(&io_conf_SDA_input);
}

bool read_ACK_NACK_slave() {
	while (!SCL_rising)
		;
	bool ack = gpio_get_level(I2C_SDA);
	SCL_rising = 0;
	return ack;
}

void i2c_task_slave(void *pvParameters) {
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

	gpio_config_t io_conf_SCL_input;
	io_conf_SCL_input.mode = GPIO_MODE_INPUT;          // Set as output mode
	io_conf_SCL_input.pin_bit_mask = (1ULL << I2C_SCL); // Set both SDA and SCL
	io_conf_SCL_input.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
	io_conf_SCL_input.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf_SCL_input.intr_type = GPIO_INTR_POSEDGE;

	gpio_config(&io_conf_SCL_input);

	io_conf_SDA_input.intr_type = GPIO_INTR_NEGEDGE;
	io_conf_SDA_input.pull_down_en = GPIO_PULLDOWN_ENABLE;

	gpio_config(&io_conf_SDA_input);

	gpio_install_isr_service(0);

	gpio_isr_handler_add(I2C_SDA, SDA_falling_isr_handler, NULL);

	gpio_set_level(I2C_SCL, 1);
	gpio_set_level(I2C_SDA, 1);

	xTaskCreate(i2c_task_slave, "i2c_task_slave", 6048, NULL,
	configMAX_PRIORITIES - 1, NULL);
}
