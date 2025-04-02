#ifndef COMPONENTS_TEST_UART_TESTS_H_
#define COMPONENTS_TEST_UART_TESTS_H_

#include "esp_err.h"
#include <stdbool.h>

#define IRAM_ATTR __attribute__((section(".iram1")));
// Fake GPIO constants
#define GPIO_MODE_INPUT  0
#define GPIO_PULLUP_ENABLE  1
#define GPIO_PULLDOWN_DISABLE 0
#define configSTACK_DEPTH_TYPE uint16_t
#define UBaseType_t   int
#define BaseType_t   int
#define configMAX_PRIORITIES		15
#define SERIAL_MONITOR_BAUD_RATE 115200
#define RX_PIN 12  // GPIO3 (RX) for UART reception
#define BAUD_RATE 9600
#define BIT_TIME_US (1000000 / BAUD_RATE) // Time per bit in microseconds
#define BUFFER_SIZE 128

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1
#define pdFALSE			( ( BaseType_t ) 0 )
#define pdTRUE			( ( BaseType_t ) 1 )

#define pdPASS			( pdTRUE )
#define pdFAIL			( pdFALSE )

// Fake UART hardware flow control (RTS/CTS)

typedef struct {
	unsigned long long pin_bit_mask;  // Bit mask for the GPIO pins to configure
	int mode;                      // Mode of the GPIO pin (input, output, etc.)
	int pull_up_en;                    // Enable pull-up resistor
	int pull_down_en;                  // Enable pull-down resistor
	int intr_type;           // Interrupt type (rising edge, falling edge, etc.)
} gpio_config_t;


typedef struct {
	int uart_nr;    // UART number (e.g., UART0, UART1)
	int rx_pin;     // RX pin number (ignored for UART1)
	int tx_pin;     // TX pin number
	int tx_enabled; // Flag indicating if TX is enabled
	int rx_enabled; // Flag indicating if RX is enabled
	int baud_rate;  // Baud rate
} uart_t;

// Fake UART byte size
typedef enum {
	UART_DATA_5_BITS = 5,
	UART_DATA_6_BITS = 6,
	UART_DATA_7_BITS = 7,
	UART_DATA_8_BITS = 8  // Default
} uart_word_length_t;

// Fake UART parity mode
typedef enum {
	UART_PARITY_DISABLE = 0,  // No parity
	UART_PARITY_EVEN = 1,     // Even parity
	UART_PARITY_ODD = 2       // Odd parity
} uart_parity_t;

// Fake UART stop bits
typedef enum {
	UART_STOP_BITS_1 = 1, UART_STOP_BITS_1_5 = 2, UART_STOP_BITS_2 = 3
} uart_stop_bits_t;

typedef enum {
	UART_HW_FLOWCTRL_DISABLE = 0,
	UART_HW_FLOWCTRL_RTS = 1,
	UART_HW_FLOWCTRL_CTS = 2,
	UART_HW_FLOWCTRL_CTS_RTS = 3
} uart_hw_flowcontrol_t;

typedef enum {
	UART_NUM_0 = 0x0, UART_NUM_1 = 0x1, UART_NUM_MAX,
} uart_port_t;

typedef enum {
	GPIO_INTR_DISABLE = 0, /*!< Disable GPIO interrupt */
	GPIO_INTR_POSEDGE = 1, /*!< GPIO interrupt type : rising edge */
	GPIO_INTR_NEGEDGE = 2, /*!< GPIO interrupt type : falling edge */
	GPIO_INTR_ANYEDGE = 3, /*!< GPIO interrupt type : both rising and falling edge */
	GPIO_INTR_LOW_LEVEL = 4, /*!< GPIO interrupt type : input low level trigger */
	GPIO_INTR_HIGH_LEVEL = 5, /*!< GPIO interrupt type : input high level trigger */
	GPIO_INTR_MAX,
} gpio_int_type_t;

typedef enum {
	GPIO_NUM_0 = 0, /*!< GPIO0, input and output */
	GPIO_NUM_1 = 1, /*!< GPIO1, input and output */
	GPIO_NUM_2 = 2, /*!< GPIO2, input and output */
	GPIO_NUM_3 = 3, /*!< GPIO3, input and output */
	GPIO_NUM_4 = 4, /*!< GPIO4, input and output */
	GPIO_NUM_5 = 5, /*!< GPIO5, input and output */
	GPIO_NUM_6 = 6, /*!< GPIO6, input and output */
	GPIO_NUM_7 = 7, /*!< GPIO7, input and output */
	GPIO_NUM_8 = 8, /*!< GPIO8, input and output */
	GPIO_NUM_9 = 9, /*!< GPIO9, input and output */
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

// Fake UART configuration struct
typedef struct {
	int baud_rate; /*!< UART baud rate*/
	uart_word_length_t data_bits; /*!< UART byte size*/
	uart_parity_t parity; /*!< UART parity mode*/
	uart_stop_bits_t stop_bits; /*!< UART stop bits*/
	uart_hw_flowcontrol_t flow_ctrl; /*!< UART HW flow control mode (CTS/RTS)*/
	uint8_t rx_flow_ctrl_thresh; /*!< UART HW RTS threshold*/
} uart_config_t;

typedef void *QueueHandle_t;
typedef void (*TaskFunction_t)(void*);
typedef void *TaskHandle_t;
typedef void (*gpio_isr_t)(void*);

extern volatile bool start_bit_detected;
extern bool stop_bit;
extern uint8_t received_data[BUFFER_SIZE];
extern esp_err_t mock_uart_driver_install_result ;
extern esp_err_t mock_gpio_config_result;
extern esp_err_t mock_gpio_isr_handler_add_result;
extern esp_err_t mock_gpio_install_isr_service_result ;
extern esp_err_t mock_uart_param_config_result ;

extern BaseType_t mock_xTaskCreate_result;
// Fake GPIO functions
esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num);

esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler,
		void *args);

int gpio_get_level(gpio_num_t gpio_num);

// Fake delay function
void ets_delay_us(int delay);
// Fake watchdog reset
void esp_task_wdt_reset();

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char *const pcName,
const configSTACK_DEPTH_TYPE usStackDepth, void *const pvParameters,
UBaseType_t uxPriority, TaskHandle_t *const pxCreatedTask);

esp_err_t gpio_config(const gpio_config_t *gpio_cfg);
esp_err_t gpio_install_isr_service(int no_use);
esp_err_t my_uart_init(uart_t *uart);

void test_my_uart_init();
esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size,
		int tx_buffer_size, int queue_size, QueueHandle_t *uart_queue,
		int intr_alloc_flags);
esp_err_t uart_param_config(uart_port_t uart_num,
		const uart_config_t *uart_config);
void test_start_reciving_task(void);

#endif
