#include "my_print_test.h"

typedef int (*putchar_like_t)(int);

putchar_like_t mock_current_putchar = NULL;

// Global variables to control/mock behavior in tests
SemaphoreHandle_t last_given_semaphore = NULL;
int xSemaphoreGive_called = 0;
int xSemaphoreGive_return_value = pdTRUE; // default success
// Shared fake mutex handle used for both create and take mocks
SemaphoreHandle_t fake_mutex_handle =
    (SemaphoreHandle_t)0x1234; // dummy non-NULL pointer

// Control flags for mutex creation mock
bool simulate_mutex_creation_failure = false;
bool xSemaphoreCreateMutex_called = false;
int printf_called;

// Control variables for xSemaphoreTake mock
SemaphoreHandle_t last_taken_semaphore = NULL;
TickType_t last_block_time = 0;
int next_return_value = pdTRUE; // return value for xSemaphoreTake

// Mocks for memset
int memset_called = 0;
void *memset_last_ptr = NULL;
int memset_last_val = -1;
size_t memset_last_size = 0;

// Mocks for memcpy
int memcpy_called = 0;
void *memcpy_dest = NULL;
const void *memcpy_src = NULL;
size_t memcpy_size = 0;

// Mock control variables for vsnprintf
int vsnprintf_called = 0;
char vsnprintf_last_buffer[256];
const char *vsnprintf_last_format = NULL;
int vsnprintf_return_value = 0;

// Mock implementation of vsnprintf
int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  vsnprintf_called = 1;
  vsnprintf_last_format = format;

  // Optionally copy the formatted output into vsnprintf_last_buffer for test
  // verification Be careful: can't copy va_list, so just call the real
  // vsnprintf to capture output
  int ret = vsnprintf(str, size, format, ap);
  if (ret >= 0 && (size_t)ret < size) {
    strncpy(vsnprintf_last_buffer, str, sizeof(vsnprintf_last_buffer));
    vsnprintf_last_buffer[sizeof(vsnprintf_last_buffer) - 1] = '\0';
  } else {
    vsnprintf_last_buffer[0] = '\0';
  }

  vsnprintf_return_value = ret;
  return ret; // mimic real vsnprintf return value
}

// Reset mock state before each test
void reset_vsnprintf_mock() {
  vsnprintf_called = 0;
  vsnprintf_last_format = NULL;
  vsnprintf_last_buffer[0] = '\0';
  vsnprintf_return_value = 0;
}


void reset_memcpy_mock() {
  memcpy_called = 0;
  memcpy_dest = NULL;
  memcpy_src = NULL;
  memcpy_size = 0;
}

// Mock implementation of memset
void *memset(void *s, int c, size_t n) {
  memset_called = 1;
  memset_last_ptr = s;
  memset_last_val = c;
  memset_last_size = n;

  // Optional zeroing simulation
  if (s == (void *)log_buffer && c == 0) {
    // for (size_t i = 0; i < n; i++) ((char*)s)[i] = 0;
  }

  return s;
}

void reset_memset_mock() {
  memset_called = 0;
  memset_last_ptr = NULL;
  memset_last_val = -1;
  memset_last_size = 0;
}

// --- Mock of xSemaphoreCreateMutex ---
SemaphoreHandle_t xSemaphoreCreateMutex(void) {
  xSemaphoreCreateMutex_called = true;
  if (simulate_mutex_creation_failure)
    return NULL;            // simulate failure
  return fake_mutex_handle; // success returns shared fake handle
}

void mock_xSemaphoreCreateMutex_reset(void) {
  xSemaphoreCreateMutex_called = false;
  simulate_mutex_creation_failure = false;
  fake_mutex_handle = (SemaphoreHandle_t)0x1234;
}

void mock_xSemaphoreCreateMutex_set_failure(bool fail) {
  simulate_mutex_creation_failure = fail;
}

// --- Mock of xSemaphoreTake ---
int mock_xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime) {
  last_taken_semaphore = xSemaphore;
  last_block_time = xBlockTime;

  // Only allow take on the known fake_mutex_handle to simulate proper behavior
  if (xSemaphore != fake_mutex_handle)
    return pdFALSE; // fail if unknown semaphore handle

  return next_return_value;
}

void mock_xSemaphoreTake_reset(void) {
  last_taken_semaphore = NULL;
  last_block_time = 0;
  next_return_value = pdTRUE;
}

void mock_xSemaphoreTake_set_return(int ret) { next_return_value = ret; }

// --- Mock of xSemaphoreGive ---
int xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
  xSemaphoreGive_called = 1;
  last_given_semaphore = xSemaphore;

  // Only allow give on the known fake_mutex_handle to simulate proper behavior
  if (xSemaphore != fake_mutex_handle)
    return pdFALSE; // fail if unknown semaphore handle

  return xSemaphoreGive_return_value;
}

void mock_xSemaphoreGive_reset(void) {
  xSemaphoreGive_called = 0;
  last_given_semaphore = NULL;
  xSemaphoreGive_return_value = pdTRUE;
}

void mock_xSemaphoreGive_set_return(int ret) {
  xSemaphoreGive_return_value = ret;
}

// Control return value for the mock
esp_err_t mock_nvs_flash_init_return_value = ESP_OK;

// Optional: count how many times it was called
int nvs_flash_init_called = 0;

// Mock implementation
esp_err_t nvs_flash_init(void) {
  nvs_flash_init_called++;
  printf("Mock nvs_flash_init() called. Returning: 0x%X\n",
         mock_nvs_flash_init_return_value);
  return mock_nvs_flash_init_return_value;
}

// Reset mock state (optional)
void reset_nvs_flash_init_mock(void) {
  nvs_flash_init_called = 0;
  mock_nvs_flash_init_return_value = ESP_OK;
}

putchar_like_t esp_log_set_putchar(putchar_like_t func) {
  putchar_like_t old_func = mock_current_putchar;
  mock_current_putchar = func;
  return old_func;
}

// In exactly one .c file (e.g., my_print_test.c):
void va_start(va_list args, const char *format) {
  (void)args;
  (void)format;
}

void va_end(va_list args) { (void)args; }

// Test xSemaphoreCreateMutex success path
void test_xSemaphoreCreateMutex_success(void) {
  SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
  TEST_ASSERT_NOT_NULL_MESSAGE(mutex, "Mutex creation should succeed");
  TEST_ASSERT_TRUE_MESSAGE(xSemaphoreCreateMutex_called,
                           "Mutex creation function should be called");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(fake_mutex_handle, mutex,
                                "Mutex handle should match the fake handle");
}

// Test xSemaphoreCreateMutex failure path
void test_xSemaphoreCreateMutex_failure(void) {
  mock_xSemaphoreCreateMutex_set_failure(true);
  SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
  TEST_ASSERT_NULL_MESSAGE(mutex, "Mutex creation should fail and return NULL");
  TEST_ASSERT_TRUE_MESSAGE(xSemaphoreCreateMutex_called,
                           "Mutex creation function should be called");
  mock_xSemaphoreCreateMutex_set_failure(false);
}

// Test xSemaphoreTake success
void test_xSemaphoreTake_success(void) {
  mock_xSemaphoreTake_set_return(pdTRUE);
  int result = mock_xSemaphoreTake(fake_mutex_handle, 10);
  TEST_ASSERT_EQUAL_INT_MESSAGE(pdTRUE, result,
                                "xSemaphoreTake should succeed");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(fake_mutex_handle, last_taken_semaphore,
                                "Semaphore handle passed should match");
  TEST_ASSERT_EQUAL_UINT_MESSAGE(10, last_block_time,
                                 "Block time passed should match");
}

// Test xSemaphoreTake failure with unknown semaphore
void test_xSemaphoreTake_fail_unknown_semaphore(void) {
  int result = mock_xSemaphoreTake((SemaphoreHandle_t)0xdeadbeef, 5);
  TEST_ASSERT_EQUAL_INT_MESSAGE(
      pdFALSE, result, "xSemaphoreTake should fail for unknown semaphore");
}

// Test xSemaphoreGive success
void test_xSemaphoreGive_success(void) {
  mock_xSemaphoreGive_set_return(pdTRUE);
  int result = xSemaphoreGive(fake_mutex_handle);
  TEST_ASSERT_EQUAL_INT_MESSAGE(pdTRUE, result,
                                "xSemaphoreGive should succeed");
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, xSemaphoreGive_called,
                                "xSemaphoreGive should be called once");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(fake_mutex_handle, last_given_semaphore,
                                "Semaphore handle given should match");
}

// Test xSemaphoreGive failure with unknown semaphore
void test_xSemaphoreGive_fail_unknown_semaphore(void) {
  int result = xSemaphoreGive((SemaphoreHandle_t)0xdeadbeef);
  TEST_ASSERT_EQUAL_INT_MESSAGE(
      pdFALSE, result, "xSemaphoreGive should fail for unknown semaphore");
}

// Test memcpy mock behavior
void test_memcpy_called_and_params(void) {
  char dest[10];
  const char src[10] = "abcdefghij";
  void *ret = memcpy(dest, src, 10);


  TEST_ASSERT_EQUAL_UINT32_MESSAGE(10, memcpy_size, "memcpy size should match");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(dest, ret,
                                "memcpy should return destination pointer");
  TEST_ASSERT_EQUAL_CHAR_ARRAY_MESSAGE(src, dest, 10,
                                       "memcpy should copy correct data");
}

// Test memset mock behavior
void test_memset_called_and_params(void) {
  char buffer[10];
  void *ret = memset(buffer, 'A', 10);

  TEST_ASSERT_EQUAL_INT_MESSAGE(1, memset_called, "memset should be called");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, memset_last_ptr,
                                "memset pointer should match");
  TEST_ASSERT_EQUAL_INT_MESSAGE('A', memset_last_val,
                                "memset value should match");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(10, memset_last_size,
                                   "memset size should match");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, ret,
                                "memset should return pointer passed");
}

void test_vsnprintf_called_and_output(void) {
  char buffer[50];
  const char *format = "Test number %d";
  int number = 42;

  reset_vsnprintf_mock();

  // Call snprintf directly instead of messing with va_list
  int ret = snprintf(buffer, sizeof(buffer), format, number);

  TEST_ASSERT_TRUE_MESSAGE(vsnprintf_called, "vsnprintf should be called");
  UNITY_TEST_ASSERT_EQUAL_STRING(format, vsnprintf_last_format, __LINE__,
                                 "Format string should match");
  TEST_ASSERT_GREATER_OR_EQUAL_INT_MESSAGE(
      0, ret, "vsnprintf return value should be non-negative");
  UNITY_TEST_ASSERT_EQUAL_STRING(buffer, vsnprintf_last_buffer, __LINE__,
                                 "Buffer contents should match last buffer");
}

// Test nvs_flash_init success and failure
void test_nvs_flash_init_mock_behavior(void) {
  reset_nvs_flash_init_mock();
  mock_nvs_flash_init_return_value = ESP_OK;
  esp_err_t ret = nvs_flash_init();
  TEST_ASSERT_EQUAL_INT_MESSAGE(ESP_OK, ret,
                                "nvs_flash_init should return ESP_OK");
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, nvs_flash_init_called,
                                "nvs_flash_init should be called once");

  reset_nvs_flash_init_mock();
  mock_nvs_flash_init_return_value = ESP_ERR_NO_MEM;
  ret = nvs_flash_init();
  TEST_ASSERT_EQUAL_INT_MESSAGE(ESP_ERR_NO_MEM, ret,
                                "nvs_flash_init should return ESP_ERR_NO_MEM");
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, nvs_flash_init_called,
                                "nvs_flash_init should be called once again");
}

// Test my_print_init creates mutex and sets putchar
void test_my_print_init_success(void) {
  my_print_init();

  TEST_ASSERT_TRUE(xSemaphoreCreateMutex_called);
  TEST_ASSERT_NOT_NULL(log_mutex);
  TEST_ASSERT_EQUAL_PTR(fake_mutex_handle, log_mutex);
  TEST_ASSERT_EQUAL_PTR(my_custom_putchar, mock_current_putchar);
}

// Test my_print_init prints error if mutex creation fails
void test_my_print_init_mutex_failure_prints_error(void) {
  simulate_mutex_creation_failure = true;

  // Capture output by resetting a printf buffer in your mock
  int printf_called;
  char printf_buffer[512];
  printf_called = 0;
  memset(printf_buffer, 0, sizeof(printf_buffer));

  my_print_init();
  TEST_ASSERT_NOT_NULL(strstr(printf_buffer, "Failed to create log mutex!"));
}

// Test my_custom_putchar forwards character correctly to my_print
void test_my_custom_putchar_calls_my_print(void) {
  // Override my_print to capture its input
  char captured_print[10];
  void my_print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(captured_print, sizeof(captured_print), format, args);
    va_end(args);
  }

  int c = 'Z';
  int ret = my_custom_putchar(c);

  TEST_ASSERT_EQUAL_INT(c, ret);
  TEST_ASSERT_EQUAL_STRING("Z", captured_print);
}

// Test log_to_buffer appends string properly with mutex calls
void test_log_to_buffer_appends_and_calls_mutex(void) {
  // Setup log_mutex so log_to_buffer runs
  log_mutex = fake_mutex_handle;
  log_index = 0;

  // Reset semaphore call trackers
  xSemaphoreGive_called = 0;
  last_given_semaphore = NULL;
  last_taken_semaphore = NULL;

  const char *msg = "Hello";
  // Call log_to_buffer via my_print indirectly
  my_print("%s", msg);

  // Check that mutex was taken and given
  TEST_ASSERT_EQUAL_PTR(log_mutex, last_taken_semaphore);
  TEST_ASSERT_EQUAL_PTR(log_mutex, last_given_semaphore);
  TEST_ASSERT_TRUE(xSemaphoreGive_called > 0);

  // Check buffer contains the string appended
  TEST_ASSERT_EQUAL_STRING(msg, log_buffer);
  TEST_ASSERT_EQUAL(strlen(msg), log_index);
}

// Test log_to_buffer resets buffer on overflow
void test_log_to_buffer_resets_on_overflow(void) {
  log_mutex = fake_mutex_handle;

  // Fill buffer close to limit
  log_index = LOG_BUFFER_SIZE - 3;
  strcpy(&log_buffer[log_index], "abc");

  const char *msg = "overflow";

  // Call log_to_buffer via my_print indirectly
  my_print("%s", msg);

  // After overflow, index reset to length of new message
  TEST_ASSERT_EQUAL(strlen(msg), log_index);

  // Buffer starts fresh with new message
  TEST_ASSERT_EQUAL_MEMORY(msg, log_buffer, strlen(msg));

  // Check memset was called (resetting buffer)
  TEST_ASSERT_TRUE(memset_called > 0);
  TEST_ASSERT_EQUAL_PTR(log_buffer, memset_last_ptr);
  TEST_ASSERT_EQUAL(0, memset_last_val);
  TEST_ASSERT_EQUAL(sizeof(log_buffer), memset_last_size);
}

// Test my_print formats string correctly and calls printf
void test_my_print_formats_and_prints(void) {
  // Reset mocks
  vsnprintf_called = 0;
  vsnprintf_last_buffer[0] = '\0';

  printf_called = 0;

  char printf_buffer[128] = {0}; // adjust size if needed

  memset(printf_buffer, 0, sizeof(printf_buffer));

  const char *format = "Value: %d";
  int val = 42;

  my_print(format, val);

  // Check vsnprintf called with correct format
  TEST_ASSERT_EQUAL_STRING(format, vsnprintf_last_format);

  // Check vsnprintf wrote to buffer
  TEST_ASSERT_TRUE(strlen(vsnprintf_last_buffer) > 0);

  // Check printf called with formatted string
  TEST_ASSERT_TRUE(printf_called > 0);
  TEST_ASSERT_TRUE(strstr(printf_buffer, "Value: 42") != NULL);

  // Check buffer updated by log_to_buffer (log_index non-zero)
  TEST_ASSERT_TRUE(log_index > 0);
  TEST_ASSERT_TRUE(strstr(log_buffer, "Value: 42") != NULL);
}

// Test my_print does nothing if log_mutex is NULL
void test_log_to_buffer_no_mutex_no_append(void) {
  log_mutex = NULL;
  log_index = 0;
  memset(log_buffer, 'X', sizeof(log_buffer));

  // Direct call to my_print with some text
  my_print("Test");

  // log_buffer unchanged since log_mutex is NULL
  TEST_ASSERT_TRUE(memcmp(log_buffer, "X", 1) == 0 || log_index == 0);
}

void Debugging_test(void) {
    RUN_TEST(test_xSemaphoreCreateMutex_success);
    RUN_TEST(test_xSemaphoreCreateMutex_failure);
    RUN_TEST(test_xSemaphoreTake_success);
    RUN_TEST(test_xSemaphoreTake_fail_unknown_semaphore);
    RUN_TEST(test_xSemaphoreGive_success);
    RUN_TEST(test_xSemaphoreGive_fail_unknown_semaphore);
    RUN_TEST(test_nvs_flash_init_mock_behavior);
}