#include "../components/UART/uart.h"
#include "../components/debugging/my_print.h"
#include "../components/web/web.h"

void app_main(void) {

	uart_t uart0 = { 0, 3, 1, 1, 0, 115200 }; // UART0 (TX: GPIO1, RX: GPIO3) @ 115200 baud
	my_uart_init(&uart0);
	my_print_init();

	my_print("Starting WiFi...\n");
	wifi_init_sta();
	my_print("WiFi started!\n");
	int i = 0;

	while (1) {
		i++;
		my_print("WiFi started! The value of i is: %d\n", i);
		vTaskDelay(10);
	}
}
