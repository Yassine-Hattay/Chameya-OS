
/**
 * @file main.c
 * @author Hattay yassine (hattayyassine519@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "../components/UART/uart.h"
#include "../components/debugging/my_print.h"
#include "../components/web/web.h"
#include "../components/ILI9488_driver/ILI9488_driver.h"
#include "../components/tasks/tasks.h"

void app_main(void) {

	uart_t uart0 = { 0, 3, 1, 1, 0, 115200 }; // UART0 (TX: GPIO1, RX: GPIO3) @ 115200 baud

	my_uart_init(&uart0);

	esp_set_cpu_freq(ESP_CPU_FREQ_160M);  // Set CPU speed to 160 MHz

	// format_spiffs();

	init_display();

	set_orientation(1);

	//FillScreenBlue();
	FillScreenblack();

	set_resolution_pos(150, 130, 73, 47, 0);

	send_command(0x3A); // interface pixel format
	send_ILI9488_data(0x06);

	send_command(0x2C);

	for (uint64_t i = 0; i < size_var; i++) {
		send_ILI9488_data(chameya[i]);
	}

	background_color = "black";

	print_ILI9488("-OS", 225, 135, 2);

	vTaskDelay(1000);

	clean_screen();

	gpio_set_level(SS_display, 1);

	init_XPT2046();

	gpio_install_isr_service(0);

	xTaskCreate(main_menu_task, "main_menu_task", 1024,
	NULL, 5, &main_menu_Handle);

	xTaskCreate(memory_monitor_task, "memory_monitor_task", 1024,
	NULL, 5, &other_task_handel);

}
