#include "unity.h"
#include "UART_tests.h"

esp_err_t mock_uart_param_config_result = ESP_OK;
esp_err_t mock_uart_driver_install_result = ESP_OK;
esp_err_t mock_gpio_config_result = ESP_OK;
esp_err_t mock_gpio_isr_handler_add_result = ESP_OK;
esp_err_t mock_gpio_install_isr_service_result = ESP_OK;
BaseType_t mock_xTaskCreate_result = pdPASS;

// Unity setup functions (empty but required)
void setUp(void) {

}
void tearDown(void) {
}

// Main function to run tests
int main(void) {
	setUp();
	UNITY_BEGIN();
	RUN_TEST(test_my_uart_init);
	RUN_TEST(test_start_reciving_task);

	return UNITY_END();
}
