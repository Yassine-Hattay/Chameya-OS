#ifndef MOCKS_FAKES_H
#define MOCKS_FAKES_H

#include <string.h>
#include "unity.h"
#include <stdbool.h>
#include "unity.h"

#define pdTRUE  1
#define pdFALSE 0
// Mock macro redefinition
#undef xSemaphoreTake
#define xSemaphoreTake(xSemaphore, xBlockTime) mock_xSemaphoreTake((xSemaphore), (xBlockTime))

#define MOSI 13
#define MISO 12
#define SCK 14
#define SS 15
#define pdMS_TO_TICKS(x) (x) // Dummy macro to prevent warnings
#define IRAM_ATTR                                                              \
  __attribute__((section(".iram1"))) // Correct way to specify IRAM section
#define configSTACK_DEPTH_TYPE uint16_t
#define UBaseType_t int
#define BaseType_t int


/* Definitions for error constants. */
#define ESP_OK          0       /*!< esp_err_t value indicating success (no error) */
#define ESP_FAIL        -1      /*!< Generic esp_err_t code indicating failure */

#define ESP_ERR_NO_MEM              0x101   /*!< Out of memory */
#define ESP_ERR_INVALID_ARG         0x102   /*!< Invalid argument */
#define ESP_ERR_INVALID_STATE       0x103   /*!< Invalid state */
#define ESP_ERR_INVALID_SIZE        0x104   /*!< Invalid size */
#define ESP_ERR_NOT_FOUND           0x105   /*!< Requested resource not found */
#define ESP_ERR_NOT_SUPPORTED       0x106   /*!< Operation or feature not supported */
#define ESP_ERR_TIMEOUT             0x107   /*!< Operation timed out */
#define ESP_ERR_INVALID_RESPONSE    0x108   /*!< Received response was invalid */
#define ESP_ERR_INVALID_CRC         0x109   /*!< CRC or checksum was invalid */
#define ESP_ERR_INVALID_VERSION     0x10A   /*!< Version was invalid */
#define ESP_ERR_INVALID_MAC         0x10B   /*!< MAC address was invalid */

#define ESP_ERR_WIFI_BASE           0x3000  /*!< Starting number of WiFi error codes */
#define ESP_ERR_MESH_BASE           0x4000  /*!< Starting number of MESH error codes */

#define portMAX_DELAY 0xFFFFFFFF

typedef int32_t esp_err_t;

#define ESP_ERROR_CHECK(x) do {                      \
    esp_err_t __err_rc = (x);                        \
    if (__err_rc == ESP_OK) {                        \
        printf("ESP_ERROR_CHECK: OK\n");             \
    } else {                                         \
        printf("ESP_ERROR_CHECK: ERROR %d\n", __err_rc); \
    }                                                \
} while(0)

typedef enum {
  GPIO_INTR_DISABLE = 0, /*!< Disable GPIO interrupt */
  GPIO_INTR_POSEDGE = 1, /*!< GPIO interrupt type : rising edge */
  GPIO_INTR_NEGEDGE = 2, /*!< GPIO interrupt type : falling edge */
  GPIO_INTR_ANYEDGE =
      3, /*!< GPIO interrupt type : both rising and falling edge */
  GPIO_INTR_LOW_LEVEL = 4, /*!< GPIO interrupt type : input low level trigger */
  GPIO_INTR_HIGH_LEVEL =
      5, /*!< GPIO interrupt type : input high level trigger */
  GPIO_INTR_MAX,
} gpio_int_type_t;

typedef enum {
  GPIO_NUM_0 = 0,   /*!< GPIO0, input and output */
  GPIO_NUM_1 = 1,   /*!< GPIO1, input and output */
  GPIO_NUM_2 = 2,   /*!< GPIO2, input and output */
  GPIO_NUM_3 = 3,   /*!< GPIO3, input and output */
  GPIO_NUM_4 = 4,   /*!< GPIO4, input and output */
  GPIO_NUM_5 = 5,   /*!< GPIO5, input and output */
  GPIO_NUM_6 = 6,   /*!< GPIO6, input and output */
  GPIO_NUM_7 = 7,   /*!< GPIO7, input and output */
  GPIO_NUM_8 = 8,   /*!< GPIO8, input and output */
  GPIO_NUM_9 = 9,   /*!< GPIO9, input and output */
  GPIO_NUM_10 = 10, /*!< GPIO10, input and output */
  GPIO_NUM_11 = 11, /*!< GPIO11, input and output */
  GPIO_NUM_12 = 12, /*!< GPIO12, input and output */
  GPIO_NUM_13 = 13, /*!< GPIO13, input and output */
  GPIO_NUM_14 = 14, /*!< GPIO14, input and output */
  GPIO_NUM_15 = 15, /*!< GPIO15, input and output */
  GPIO_NUM_16 = 16, /*!< GPIO16, input and output */
  GPIO_NUM_MAX = 17,
  /** @endcond */
} gpio_num_t;

typedef enum {
  GPIO_MODE_DISABLE = 0,   /*!< GPIO mode: disable input and output */
  GPIO_MODE_INPUT = 1,     /*!< GPIO mode: input only */
  GPIO_MODE_OUTPUT = 2,    /*!< GPIO mode: output only */
  GPIO_MODE_OUTPUT_OD = 3, /*!< GPIO mode: output only with open-drain mode */
} gpio_mode_t;

typedef enum {
  GPIO_PULLUP_DISABLE = 0x0, /*!< Disable GPIO pull-up resistor */
  GPIO_PULLUP_ENABLE = 0x1,  /*!< Enable GPIO pull-up resistor */
} gpio_pullup_t;

typedef enum {
  GPIO_PULLDOWN_DISABLE = 0x0, /*!< Disable GPIO pull-down resistor */
  GPIO_PULLDOWN_ENABLE = 0x1,  /*!< Enable GPIO pull-down resistor  */
} gpio_pulldown_t;

typedef struct {
  uint32_t
      pin_bit_mask; /*!< GPIO pin: set with bit mask, each bit maps to a GPIO */
  gpio_mode_t mode; /*!< GPIO mode: set input/output mode */
  gpio_pullup_t pull_up_en;     /*!< GPIO pull-up */
  gpio_pulldown_t pull_down_en; /*!< GPIO pull-down */
  gpio_int_type_t intr_type;    /*!< GPIO interrupt type */
} gpio_config_t;

typedef void (*gpio_isr_t)(void *);
typedef void *QueueHandle_t;
typedef void (*TaskFunction_t)(void *);
typedef void *TaskHandle_t;
typedef void (*gpio_isr_t)(void *);

void setUp_spi(void);
void esp_task_wdt_reset();
esp_err_t gpio_install_isr_service(int no_use);
esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler,
                               void *args);

esp_err_t gpio_config(const gpio_config_t *gpio_cfg);

typedef unsigned int TickType_t; // Adjust the type size as needed

void vTaskDelay(const TickType_t xTicksToDelay);

int gpio_get_level(gpio_num_t gpio_num);
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);
void ets_delay_us(uint32_t us);

typedef void *QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;

esp_err_t nvs_flash_init(void);


#endif // MOCKS_FAKES_H
