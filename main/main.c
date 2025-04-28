#include "../components/UART/uart.h"
#include "../components/debugging/my_print.h"
#include "../components/web/web.h"
#include "../components/ILI9488_driver/ILI9488_driver.h"
#include "../components/tasks/tasks.h"

void app_main(void) {

	uart_t uart0 = { 0, 3, 1, 1, 0, 115200 }; // UART0 (TX: GPIO1, RX: GPIO3) @ 115200 baud

	my_uart_init(&uart0);

	printf("main1");

	esp_set_cpu_freq(ESP_CPU_FREQ_160M);  // Set CPU speed to 160 MHz

	//format_spiffs();

	init_display();
	printf("main");

	set_orientation(1);
	printf("main");

	//FillScreenBlue();
	FillScreenblack();
	printf("main");

	//
	//send_command(0x2C);
	//
	//for (uint64_t i = 0; i < size_var; i++) {
	//	send_ILI9488_data(chameya[i]);
	//}

	gpio_set_level(SS_display, 1);
	printf("main");

	init_XPT2046();
	printf("main");

	gpio_install_isr_service(0);
	printf("main");

	gpio_isr_handler_add(PIRQ_pin, PIRQ_isr_handler, NULL);

	printf("main9 \n");

	xTaskCreate(pong_game_task, "pong_game_task", 2048,
	NULL, 5, &main_menu_Handle);

}

