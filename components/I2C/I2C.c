#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp8266/gpio_struct.h"
#include "driver/gpio.h"

#define I2C_SDA 4  // GPIO for SDA
#define I2C_SCL 5  // GPIO for SCL
#define I2C_SLAVE_ADDR 0x42  // ESP32 Slave Address
#define I2C_DELAY_US 10  // Delay in microseconds

gpio_config_t io_conf_SDA_input = { .pin_bit_mask = (1ULL << I2C_SDA),
		.mode = GPIO_MODE_INPUT, // MOSI, SCK, and SS as input
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE // Interrupt on falling edge
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

void i2c_send_byte(uint8_t data) {
	for (int i = 7; i >= 0; i--) {
		i2c_send_bit((data >> i) & 1);

	}
	gpio_config(&io_conf_SDA_input);
}

void ACK() {
	if (!gpio_get_level(I2C_SDA)) {
		printf("ACK = 1");
	}

	gpio_config(&io_conf_SDA_output);
}

void i2c_task(void *pvParameters) {
	while (1) {
		i2c_start();
		i2c_send_byte(I2C_SLAVE_ADDR << 1);
		one_tick();
		ACK();
		i2c_send_byte('H');
		one_tick();
		ACK();

		i2c_send_byte('e');
		one_tick();
		ACK();

		i2c_send_byte('l');
		one_tick();
		ACK();

		i2c_send_byte('l');
		one_tick();
		ACK();

		i2c_send_byte('o');
		one_tick();
		ACK();

		i2c_send_byte('!');
		one_tick();
		ACK();

		i2c_stop();

		vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
	}
}

void I2C_init() {
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;     // Disable interrupt
	io_conf.mode = GPIO_MODE_OUTPUT;           // Set as output mode
	io_conf.pin_bit_mask = (1ULL << I2C_SDA) | (1ULL << I2C_SCL); // Set both SDA and SCL
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // Disable pull-up
	gpio_config(&io_conf);                      // Apply configuration

	xTaskCreate(i2c_task, "i2c_task", 2048, NULL, 5, NULL);
}
