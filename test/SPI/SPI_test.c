#include "unity.h"
#include "SPI_test.h"

uint8_t simlulated_data;
int i = 8;
// Global variables to track GPIO values (simulate the state of the pins)
// We use an array of 17 elements to simulate all GPIOs from 0 to 16
static uint32_t gpio_state[GPIO_NUM_MAX] = { 0 };

void vTaskDelay(const TickType_t xTicksToDelay) {
}
esp_err_t gpio_config(const gpio_config_t *gpio_cfg) {
}
void esp_task_wdt_reset() {
}
;

// Fake function to simulate gpio_get_level (return the value of the GPIO state)
int gpio_get_level(gpio_num_t gpio_num) {
	i = i - 1;
	return (int) (simlulated_data >> i) & 1;
}

esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler,
		void *args) {
}

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char *const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
const configSTACK_DEPTH_TYPE usStackDepth, void *const pvParameters,
UBaseType_t uxPriority, TaskHandle_t *const pxCreatedTask) {
}

esp_err_t gpio_install_isr_service(int no_use) {
}
// Fake function to simulate gpio_set_level (set the value of the GPIO state)
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) {
	gpio_state[gpio_num] = level;  // Simulate setting the GPIO level
	return ESP_OK;  // Return a success code (similar to the actual function)
}

// Fake delay function
void ets_delay_us(uint32_t us) {
	// Simulate a delay; you can track how many times this was called
	(void) us;  // We don't need to use 'us' for this fake
}

// Setup for the SPI test
void setUp_spi(void) {

}

// SPI test case function
void TestSPI_master_recive(void) {
	// Test data to send
	spi_master_init();

	uint8_t data_to_send = 0x00; // 10101010 in binary
	simlulated_data = 0x66;

	uint8_t received_data = spi_master_bit_bang_mode_2(data_to_send);
	i = 8;
	TEST_ASSERT_EQUAL_UINT8(0x66, received_data); // Since MISO=0x55, we should get that back

	simlulated_data = 0x77;

	received_data = spi_master_bit_bang_mode_2(data_to_send);
	i = 8;

	TEST_ASSERT_EQUAL_UINT8(0x77, received_data); // Since MISO=0x55, we should get that back

	simlulated_data = 0xF5;

	received_data = spi_master_bit_bang_mode_2(data_to_send);

	TEST_ASSERT_EQUAL_UINT8(0xF5, received_data);
}

