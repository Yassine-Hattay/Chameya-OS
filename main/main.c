#include "../components/UART/UART.h"
#include "../components/ILI9488_driver/ILI9488_driver.h"
#include "../components/XPT2046_driver/XPT2046_driver.h"
#include "../components/tasks/tasks.h"

#include "esp_system.h"

char *background_color;




void app_main() {

	esp_set_cpu_freq(ESP_CPU_FREQ_160M);  // Set CPU speed to 160 MHz
	uart_t uart0 = { 0, 3, 1, 1, 0, 115200 }; // UART0 (TX: GPIO1, RX: GPIO3) @ 115200 baud

	my_uart_init(&uart0);

	init_display();

	set_orientation(1);

	FillScreenBlue();

	set_resolution_pos(130, 125, 69, 39, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_var; i++) {
		send_ILI9488_data(chameya[i]);
	}

	send_command(0x00);

	background_color = "blue";
	print_ILI9488("-OS", 205, 125, 2);

	send_command(0x00);

	vTaskDelay(3500);

	clean_screen();

	set_resolution_pos(10, 10, 67, 76, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_var1; i++) {
		send_ILI9488_data(notebook[i]);
	}

	send_command(0x00);

	strncpy(history[coord_index - 1].app_name, "notebook",
	APP_NAME_MAX_LEN);

	gpio_set_level(SS_display, 1);

	init_XPT2046();

	gpio_install_isr_service(0);

	gpio_isr_handler_add(PIRQ_pin, PIRQ_isr_handler, NULL);

	xTaskCreate(main_menu_task, "main_menu_task", 2048, NULL, 5,
			&main_menu_Handle);

}

