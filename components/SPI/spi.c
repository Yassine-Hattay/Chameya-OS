#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define MOSI 13
#define MISO 12
#define SCK  14
#define SS   15

void spi_bit_bang_send(uint8_t data) {
    // Pull CS low to start communication
    gpio_set_level(SS, 0);
    ets_delay_us(10);

    for (int i = 7; i >= 0; i--) {
        // Set MOSI
        gpio_set_level(MOSI, (data >> i) & 1);

        // Clock high
        gpio_set_level(SCK, 1);
        ets_delay_us(10);

        // Clock low
        gpio_set_level(SCK, 0);
        ets_delay_us(10);
    }

    // Pull CS high to end communication
    gpio_set_level(SS, 1);
}

uint8_t spi_bit_bang_receive() {
    uint8_t received = 0;

    // Pull CS low to start communication
    gpio_set_level(SS, 0);
    ets_delay_us(10);

    for (int i = 7; i >= 0; i--) {
        // Clock high
        gpio_set_level(SCK, 1);
        ets_delay_us(10);

        // Read MISO
        received |= (gpio_get_level(MISO) << i);

        // Clock low
        gpio_set_level(SCK, 0);
        ets_delay_us(10);
    }

    // Pull CS high to end communication
    gpio_set_level(SS, 1);

    return received;
}

// Full-duplex SPI: Send and receive at the same time
void spi_bit_bang_full_duplex(uint8_t data_to_send, uint8_t *received_data) {
    uint8_t received = 0;

    // Pull CS low to start communication
    gpio_set_level(SS, 0);
    ets_delay_us(10);

    for (int i = 7; i >= 0; i--) {
        // Set MOSI
        gpio_set_level(MOSI, (data_to_send >> i) & 1);

        // Clock high
        gpio_set_level(SCK, 1);
        ets_delay_us(10);


        // Clock low
        gpio_set_level(SCK, 0);
        ets_delay_us(10);
    }

    // Pull CS high to end communication
    gpio_set_level(SS, 1);

    // Return received data
    *received_data = received;
}

void spi_task(void *pvParameter) {
    uint8_t data_to_send = 0xA5;  // Example data
    uint8_t received_data = 0;

    while (1) {
        // Full-Duplex Operation: Send and Receive at the same time
        spi_bit_bang_full_duplex(data_to_send, &received_data);
        printf("Received: 0x%02X\n", received_data);

        vTaskDelay(pdMS_TO_TICKS(1000));

       // // Half-Duplex Operation: Send and Receive sequentially
       // spi_bit_bang_send(data_to_send);  // Send first
       // uint8_t received = spi_bit_bang_receive();  // Then receive
       // printf("Received (Half-Duplex): 0x%02X\n", received);
       //
       // vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void user_init(void) {
	// Configure GPIO
	gpio_config_t io_conf = { .pin_bit_mask = (1ULL << MOSI) | (1ULL << SCK)
			| (1ULL << SS), .mode = GPIO_MODE_OUTPUT, .pull_up_en =
			GPIO_PULLUP_DISABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type = GPIO_INTR_DISABLE };
	gpio_config(&io_conf);

	// Configure MISO as input
	gpio_set_direction(MISO, GPIO_MODE_INPUT);

	// Default pin states
	gpio_set_level(MOSI, 0);
	gpio_set_level(SCK, 0);
	gpio_set_level(SS, 1);

	xTaskCreate(spi_task, "spi_task", 1024, NULL, 1, NULL);
}
