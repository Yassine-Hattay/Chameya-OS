#include "../components/UART/UART.h"
#include "../components/I2C/I2C.h"
#include "esp_system.h"

void app_main() {
    esp_set_cpu_freq(ESP_CPU_FREQ_160M);  // Set CPU speed to 160 MHz

    uart_t uart0 = {0, 3, 1, 1, 0, 115200};
    my_uart_init(&uart0);

    I2C_init_salve();
}
 