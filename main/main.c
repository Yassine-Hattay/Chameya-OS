#include "../components/I2C/I2C.h"
#include "../components/uart/recive_uart.h"

void app_main() {
    uart_t uart0 = {0, 3, 1, 1, 0, 115200};
    my_uart_init(&uart0);

    I2C_init();
}
