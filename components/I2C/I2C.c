#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp8266/gpio_struct.h"
#include "driver/gpio.h"
#include "I2C.h"

#define I2C_SDA 4  // GPIO for SDA
#define I2C_SCL 5  // GPIO for SCL
#define I2C_SLAVE_ADDR 0x42  // ESP32 Slave Address
#define I2C_DELAY_US 10  // Delay in microseconds

gpio_config_t io_conf_SDA_input = { .pin_bit_mask = (1ULL << I2C_SDA),
		.mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
		.pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en = GPIO_PULLDOWN_ENABLE,
		.intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
		};

gpio_config_t io_conf_SDA_output = { .pin_bit_mask = (1ULL << I2C_SDA),
		.mode = GPIO_MODE_OUTPUT, // MOSI, SCK, and SS as input
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
		};

void i2c_delay() {
	ets_delay_us(I2C_DELAY_US);  // Replace delay with precise microsecond delay
}

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

void i2c_task(void *pvParameters) {
	uint8_t buffer[BUFFER_SIZE];
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

void I2C_init() {
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;     // Disable interrupt
	io_conf.mode = GPIO_MODE_OUTPUT;           // Set as output mode
	io_conf.pin_bit_mask = (1ULL << I2C_SCL); // Set both SDA and SCL
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // Disable pull-up
	gpio_config(&io_conf);                      // Apply configuration

	gpio_config(&io_conf_SDA_output);                     // Apply configuration

	xTaskCreate(i2c_task, "i2c_task", 2048, NULL, 5, NULL);
}
