#include "../components/uart/recive_uart.h"
#include "../components/test/baud_rate_test.h"
#include "../components/SPI/spi.h"

void app_main() {
    uart_t uart0 = {0, 3, 1, 1, 0, 115200};
    my_uart_init(&uart0);

    spi_slave_init();
}
