/**
 * @file UART.c
 * @author your name (you@domain.com)
 * @brief this file contains the implementation of UART communication using
 * bit-banging
 * @version 0.1
 * @date 2025-05-28
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "UART.h"

volatile bool start_bit_detected = 0; // Flag for interrupt
// Define UART structure
bool stop_bit = 0;
uint8_t TX_PIN = 2;

/**
 * @brief Initializes a UART peripheral with specified settings.
 *
 * This function configures the parameters and installs the driver for a given
 * UART port on an ESP32 microcontroller.
 *
 * @param uart A pointer to a `uart_t` structure containing the UART number
 * (`uart_nr`) and the desired `baud_rate`.
 * @return `ESP_OK` if the UART is successfully configured and its driver is
 * installed; otherwise, returns an `esp_err_t` error code indicating the
 * failure.
 *
 * The initialization process involves:
 * 1. **Configuration Structure:** Populating a `uart_config_t` structure with:
 * - `baud_rate`: Taken from `uart->baud_rate`.
 * - `data_bits`: Set to 8 bits (`UART_DATA_8_BITS`).
 * - `parity`: Disabled (`UART_PARITY_DISABLE`).
 * - `stop_bits`: Set to 1 stop bit (`UART_STOP_BITS_1`).
 * - `flow_ctrl`: Hardware flow control disabled (`UART_HW_FLOWCTRL_DISABLE`).
 * 2. **Parameter Configuration:** Calling `uart_param_config()` to apply the
 * defined settings to the specified UART port (`uart->uart_nr`). Error checking
 * is performed.
 * 3. **Driver Installation:** Calling `uart_driver_install()` to install the
 * UART driver for the specified port. It sets up a transmit buffer of 1024
 * bytes and no receive buffer (or a small default one, depending on ESP-IDF
 * version if 0 is passed for rx_buffer_size and queue_size). No event queue is
 * used (`0, NULL, 0`). Error checking is performed.
 */

esp_err_t my_uart_init(uart_t *uart) {
  uart_config_t uart_config = {.baud_rate =
                                   uart->baud_rate, // Set custom baud rate
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  // Configure UART parameters
  esp_err_t err = uart_param_config(uart->uart_nr, &uart_config);
  if (err != ESP_OK)
    return err; // Return the error if configuration fails

  // Install UART driver
  err = uart_driver_install(uart->uart_nr, 1024, 0, 0, NULL, 0);
  if (err != ESP_OK)
    return err; // Return the error if driver installation fails

  return ESP_OK; // Return success if both operations succeed
}

// ISR to detect the start of the UART reception (start bit)
IRAM_ATTR void uart_rx_isr_handler(void *arg) { start_bit_detected = 1; }

/**
 * @brief FreeRTOS task for bit-banging UART receive.
 *
 * This task continuously monitors the `RX_PIN` for a start bit.
 * Upon detection, it samples the incoming data line at precise intervals
 * to reconstruct bytes. It then stores these bytes in a buffer and,
 * when the buffer is full or a stop condition is met, prints the received
 * string. The task manages enabling/disabling the RX interrupt during
 * reception.
 */

void uart_bitbang_receive_task(void *param) {
  uint8_t received_data[BUFFER_SIZE];
  int index = 0;
  while (1) {

    if (start_bit_detected) {
      uint8_t byte = 0;

      gpio_isr_handler_remove(RX_PIN); // Disable interrupt while receiving

      ets_delay_us(BIT_TIME_US_RX / 2); // Move to center of first data bit

      for (int i = 0; i < 8; i++) {
        ets_delay_us(BIT_TIME_US_RX);          // Wait for each bit
        byte |= (gpio_get_level(RX_PIN) << i); // Read bit and store in byte
      }
      ets_delay_us(BIT_TIME_US_RX); // Wait for each bit
      stop_bit = gpio_get_level(RX_PIN);

      if (!stop_bit) {
        ets_delay_us(BIT_TIME_US_RX * 1.1);
        index = 0;
      } else {
        if (index < BUFFER_SIZE) {
          received_data[index++] = byte;
        } else {
          printf("new : %s \n", received_data);
          index = 0;
          esp_task_wdt_reset();
        }
        stop_bit = 0;
      }
      start_bit_detected = 0;
      gpio_isr_handler_add(RX_PIN, uart_rx_isr_handler, NULL);
    }
  }
}

/**
 * @brief Initializes GPIO for UART reception and creates the bit-banging
 * receive task.
 *
 * This function configures the `RX_PIN` as an input with pull-up and sets up a
 * negative edge interrupt. It then installs the GPIO ISR service and registers
 * `uart_rx_isr_handler` to detect start bits. Finally, it creates and starts
 * the `uart_bitbang_receive_task` to handle the actual data reception.
 *
 * @return `ESP_OK` if all configurations and task creation are successful;
 * otherwise, returns an `esp_err_t` error code indicating the failure.
 */

esp_err_t start_reciving_task(void) {
  gpio_config_t io_conf = {.pin_bit_mask = (1ULL << RX_PIN),
                           .mode = GPIO_MODE_INPUT,
                           .pull_up_en = GPIO_PULLUP_ENABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_NEGEDGE};

  // GPIO configuration
  esp_err_t ret = gpio_config(&io_conf);
  if (ret != ESP_OK) {
    printf("start_reciving_task: GPIO configuration failed, error: %d\n", ret);
    return ret; // Return error code if GPIO config fails
  }

  // Install ISR service
  ret = gpio_install_isr_service(0);
  if (ret != ESP_OK) {
    printf("start_reciving_task: ISR service installation failed, error: %d\n",
           ret);
    return ret; // Return error code if ISR service fails
  }

  // Add ISR handler
  ret = gpio_isr_handler_add(RX_PIN, uart_rx_isr_handler, NULL);
  if (ret != ESP_OK) {
    printf("start_reciving_task: ISR handler addition failed, error: %d\n",
           ret);
    return ret; // Return error code if adding ISR handler fails
  }

  // Create receiving task
  BaseType_t task_create_status =
      xTaskCreate(uart_bitbang_receive_task, "uart_rx_task", 4096, NULL,
                  configMAX_PRIORITIES - 1, NULL);
  if (task_create_status != pdPASS) {
    printf("start_reciving_task: Task creation failed\n");
    return ESP_FAIL; // Return failure code if task creation fails
  }

  return ESP_OK; // Return success if everything succeeded
}

/**
 * @brief Sends a single byte over UART using bit-banging.
 *
 * This function manually controls the `TX_PIN` to transmit one byte of data
 * by simulating UART protocol:
 * - Sends a **start bit** (low).
 * - Sends 8 **data bits** (least significant bit first).
 * - Sends a **stop bit** (high).
 * Precise timing for each bit is achieved using `ets_delay_us()`.
 *
 * @param byte The 8-bit data byte to be transmitted.
 */
// Function to send a single byte via bit-banging
static void uart_bitbang_send_byte(uint8_t byte) {
  // Start bit (low)
  gpio_set_level(TX_PIN, 0);
  ets_delay_us(BIT_TIME_US_TX);

  // Send 8 data bits (LSB first)
  for (int i = 0; i < 8; i++) {
    gpio_set_level(TX_PIN, (byte >> i) & 1);
    ets_delay_us(BIT_TIME_US_TX);
  }

  // Stop bit (high)
  gpio_set_level(TX_PIN, 1);
  ets_delay_us(BIT_TIME_US_TX);
}

/**
 * @brief Sends a string over UART using bit-banging.
 *
 * This function iterates through each character of a given string and
 * transmits each character as a single byte using the `uart_bitbang_send_byte`
 * function.
 *
 * @param str A pointer to the constant character array (string) to be sent.
 * @param length The number of characters (bytes) in the string to transmit.
 */

// Function to send a string
void uart_bitbang_send_string(const char *str, size_t length) {
  for (size_t i = 0; i < length; i++)
    uart_bitbang_send_byte(str[i]);
}

/**
 * @brief FreeRTOS task for continuously sending a UART message (currently
 * commented out).
 *
 * This task is designed to run indefinitely, sending a predefined message via
 * bit-banged UART. However, the message sending logic is currently commented
 * out.
 *
 * @param param Unused parameter, required by FreeRTOS task signature.
 *
 * The task's `while(1)` loop would typically:
 * - Call `uart_bitbang_send_string()` to transmit a message.
 * - Introduce a delay using `vTaskDelay()` to control the sending interval.
 *
 * As of the current code, the task simply enters an infinite loop without
 * performing any actions.
 */

// Task to continuously send a message
static void uart_task(void *param) {
  while (1) {
    // uart_bitbang_send_string(
    //		"Hello from bit-banged UART1 on GPIO2 (D4)!\n");
    // vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second
  }
}

/**
 * @brief Initializes the GPIO for UART transmission and creates the UART send
 * task.
 *
 * This function sets up the `TX_PIN` as an output and ensures it's in the idle
 * (high) state. It then creates and starts the `uart_task`, which is
 * responsible for continuously sending data via bit-banged UART.
 */

static void init_transmit_task() {
  // Configure TX pin as output
  gpio_set_direction(TX_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(TX_PIN, 1); // Idle state is high

  // Start UART task
  xTaskCreate(uart_task, "uart_task", 2048, NULL, 1, NULL);
}
