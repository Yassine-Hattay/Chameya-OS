#include "../components/UART/UART.h"
#include "../components/ILI9488_driver/ILI9488_driver.h"
#include "../components/XPT2046_driver/XPT2046_driver.h"
#include "../components/tasks/tasks.h"

#include "esp_system.h"

void memory_monitor_task(void *pvParameters) {
	while (1) {
		printf("Current free heap: %u bytes\n", esp_get_free_heap_size());
		vTaskDelay(500);  // Every 5 seconds
	}
}

void app_main() {

	uart_t uart0 = { 0, 3, 1, 1, 0, 115200 }; // UART0 (TX: GPIO1, RX: GPIO3) @ 115200 baud

	my_uart_init(&uart0);

	esp_set_cpu_freq(ESP_CPU_FREQ_160M);  // Set CPU speed to 160 MHz

	//format_spiffs();

	init_display();

	set_orientation(1);

	//FillScreenBlue();
	FillScreenblack();

	set_resolution_pos(130, 125, 69, 39, 0);

	//send_command(0x3A); // interface pixel format
	//send_ILI9488_data(0x06);
	//
	//send_command(0x2C);
	//
	//for (uint64_t i = 0; i < size_var; i++) {
	//	send_ILI9488_data(chameya[i]);
	//}

	send_command(0x00);

	background_color = "black";
	print_ILI9488("-OS", 205, 125, 2);

	send_command(0x00);

	vTaskDelay(100);

	clean_screen();

	strncpy(history[coord_index].app_name, "notebook",
	APP_NAME_MAX_LEN);

	gpio_set_level(SS_display, 1);

	init_XPT2046();

	gpio_install_isr_service(0);

	gpio_isr_handler_add(PIRQ_pin, PIRQ_isr_handler, NULL);

	xTaskCreate(main_menu_task, "main_menu_task", 2048, NULL, 5,
			&main_menu_Handle);

	xTaskCreate(memory_monitor_task, "mem_monitor", 2048, NULL, 5, NULL);

}

