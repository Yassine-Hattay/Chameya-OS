#ifndef MY_PRINT_TEST_H
#define MY_PRINT_TEST_H

#include "mocks_fakes.h"
#define LOG_BUFFER_SIZE 4096 // Size of the RAM log buffer

extern char log_buffer[LOG_BUFFER_SIZE];
extern size_t log_index;

typedef int (*putchar_like_t)(int);

extern esp_err_t mocked_nvs_flash_return_value;
// Holds the currently set putchar function (mocked
extern putchar_like_t mock_current_putchar;

typedef int (*putchar_like_t)(int c);

esp_err_t nvs_flash_init(void);

// Mock version of esp_log_set_putchar
putchar_like_t esp_log_set_putchar(putchar_like_t func);
SemaphoreHandle_t xSemaphoreCreateMutex(void);
int mock_xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime);
int xSemaphoreGive(SemaphoreHandle_t xSemaphore);
esp_err_t nvs_flash_init(void);

void va_start(va_list args, const char *format);
void va_end(va_list args);

void mock_xSemaphoreCreateMutex_reset(void);
void mock_xSemaphoreTake_reset(void);
void mock_xSemaphoreGive_reset(void);
void reset_memcpy_mock();
void reset_memset_mock();
void reset_vsnprintf_mock();
void reset_nvs_flash_init_mock(void);

void test_xSemaphoreCreateMutex_success(void);
void test_xSemaphoreCreateMutex_failure(void);
void test_xSemaphoreTake_success(void);
void test_xSemaphoreTake_fail_unknown_semaphore(void);
void test_xSemaphoreGive_success(void);
void test_xSemaphoreGive_fail_unknown_semaphore(void);
void test_memcpy_called_and_params(void);
void test_memset_called_and_params(void);
void test_vsnprintf_called_and_output(void);
void test_nvs_flash_init_mock_behavior(void);
int my_custom_putchar(int c);
void my_print(const char *format, ...);
void Debugging_test(void);

extern SemaphoreHandle_t log_mutex;
extern bool simulate_mutex_creation_failure;
extern bool xSemaphoreCreateMutex_called;
extern SemaphoreHandle_t last_given_semaphore;
extern int xSemaphoreGive_called;
extern int xSemaphoreGive_return_value; // default success
extern SemaphoreHandle_t last_taken_semaphore;
extern TickType_t last_block_time;
extern int next_return_value; // return value for xSemaphoreTake
extern int memset_called;
extern void *memset_last_ptr;
extern int memset_last_val;
extern size_t memset_last_size;
extern int memcpy_called;
extern void *memcpy_dest;
extern const void *memcpy_src;
extern size_t memcpy_size;
extern int vsnprintf_called;
extern const char *vsnprintf_last_format;
extern int vsnprintf_return_value;
extern char vsnprintf_last_buffer[];
void my_print_init(void);

#endif /* MY_PRINT_TEST_H */
