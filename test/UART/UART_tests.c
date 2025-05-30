#include "unity.h"
#include "UART_tests.h"
// Mock variables
esp_err_t mock_uart_param_config_result = ESP_OK;
esp_err_t mock_uart_driver_install_result = ESP_OK;
esp_err_t mock_gpio_config_result = ESP_OK;
esp_err_t mock_gpio_isr_handler_add_result = ESP_OK;
esp_err_t mock_gpio_install_isr_service_result = ESP_OK;
BaseType_t mock_xTaskCreate_result = pdPASS;

// Mock function (same name as original)
esp_err_t uart_param_config(uart_port_t uart_num,
		const uart_config_t *uart_config) {
	(void) uart_num;   // Suppress unused parameter warning
	(void) uart_config; // Suppress unused parameter warning
	return mock_uart_param_config_result;
}

esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size,
		int tx_buffer_size, int queue_size, QueueHandle_t *uart_queue,
		int intr_alloc_flags) {
	(void) uart_num;
	(void) rx_buffer_size;
	(void) tx_buffer_size;
	(void) queue_size;
	(void) uart_queue;
	(void) intr_alloc_flags;
	return mock_uart_driver_install_result;
}

esp_err_t gpio_config(const gpio_config_t *gpio_cfg) {
	// Simulate behavior: if gpio_cfg is NULL, return error
	if (gpio_cfg == NULL) {
		return ESP_ERR_INVALID_ARG;  // Simulate an invalid argument error
	}

	return mock_gpio_config_result;
}

esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num) {
	// Simulate success, no actual removal logic needed for the mock
	return ESP_OK;
}

// Mock function for gpio_isr_handler_add
esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler,
		void *args) {
	// Simulate success, no actual handler addition logic needed for the mock
	return mock_gpio_isr_handler_add_result;
}

int gpio_get_level(gpio_num_t gpio_num) {
	return 0;
}  // Fake: Always return low level
void ets_delay_us(int delay) {
}  // Fake: Do nothing

void esp_task_wdt_reset() {
}  // Fake: Do nothing

esp_err_t gpio_install_isr_service(int no_use) {
	// Simulate checking the input value (if it's invalid, return an error)

	return mock_gpio_install_isr_service_result;
}

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char *const pcName,
		const configSTACK_DEPTH_TYPE usStackDepth, void *const pvParameters,
		UBaseType_t uxPriority, TaskHandle_t *const pxCreatedTask) {

	return mock_xTaskCreate_result;
}

void test_my_uart_init() {
	uart_t test_uart = { .uart_nr = 1, .baud_rate = 9600 };

	esp_err_t result = my_uart_init(&test_uart);
	TEST_ASSERT_EQUAL(ESP_OK, result);

	// Case 2: uart_param_config() fails
	mock_uart_param_config_result = ESP_FAIL;
	mock_uart_driver_install_result = ESP_OK;
	result = my_uart_init(&test_uart);
	TEST_ASSERT_EQUAL(ESP_FAIL, result);

	// Case 3: uart_driver_install() fails
	mock_uart_param_config_result = ESP_OK;
	mock_uart_driver_install_result = ESP_FAIL;
	result = my_uart_init(&test_uart);
	TEST_ASSERT_EQUAL(ESP_FAIL, result);

	// Case 4: Both functions fail
	mock_uart_param_config_result = ESP_FAIL;
	mock_uart_driver_install_result = ESP_FAIL;
	result = my_uart_init(&test_uart);
	TEST_ASSERT_EQUAL(ESP_FAIL, result);
}

void test_start_reciving_task(void) {
	// Test 1: Success case where everything works
	esp_err_t result;

	// Mock success for all dependencies
	gpio_config_t io_conf = { .pin_bit_mask = (1ULL << RX_PIN), .mode =
			GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE, .pull_down_en =
			GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_NEGEDGE };

	mock_gpio_config_result = ESP_OK;  // Simulating gpio_config success
	mock_gpio_install_isr_service_result = ESP_OK; // Simulating gpio_install_isr_service success
	mock_gpio_isr_handler_add_result = ESP_OK; // Simulating gpio_isr_handler_add success
	mock_xTaskCreate_result = pdPASS;  // Simulating success

	result = start_reciving_task();
	TEST_ASSERT_EQUAL(ESP_OK, result); // Expect success when all functions return success

	// Test 2: Failure in GPIO configuration (gpio_config)
	mock_gpio_config_result = ESP_FAIL;  // Simulating gpio_config failure
	result = start_reciving_task();
	TEST_ASSERT_EQUAL(ESP_FAIL, result); // Expect failure when gpio_config fails

	// Test 3: Failure in ISR service installation (gpio_install_isr_service)
	mock_gpio_config_result = ESP_OK;  // Simulating gpio_config success
	mock_gpio_install_isr_service_result = ESP_FAIL; // Simulating gpio_install_isr_service failure
	result = start_reciving_task();
	TEST_ASSERT_EQUAL(ESP_FAIL, result); // Expect failure when gpio_install_isr_service fails

	// Test 4: Failure in ISR handler addition (gpio_isr_handler_add)
	mock_gpio_config_result = ESP_OK;  // Simulating gpio_config success
	mock_gpio_install_isr_service_result = ESP_OK; // Simulating gpio_install_isr_service success
	mock_gpio_isr_handler_add_result = ESP_FAIL; // Simulating gpio_isr_handler_add failure
	result = start_reciving_task();
	TEST_ASSERT_EQUAL(ESP_FAIL, result); // Expect failure when gpio_isr_handler_add fails

	// Test 5: Failure in task creation (xTaskCreate)
	mock_gpio_config_result = ESP_OK;  // Simulating gpio_config success
	mock_gpio_install_isr_service_result = ESP_OK; // Simulating gpio_install_isr_service success
	mock_gpio_isr_handler_add_result = ESP_OK; // Simulating gpio_isr_handler_add success
	mock_xTaskCreate_result = pdFAIL;  // Simulating xTaskCreate failure
	result = start_reciving_task();
	TEST_ASSERT_EQUAL(ESP_FAIL, result); // Expect failure when xTaskCreate fails
}

