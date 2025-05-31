#include "SPI/SPI_test.h"
#include "debugging/my_print_test.h"

#include "unity.h"

void setUp(void) {
  // Reset globals and mocks
  log_index = 0;
  memset(log_buffer, 0,
         LOG_BUFFER_SIZE); // use LOG_BUFFER_SIZE, not sizeof(log_buffer)
  log_mutex = NULL;

  simulate_mutex_creation_failure = false;
  xSemaphoreCreateMutex_called = false;
  last_given_semaphore = NULL;
  xSemaphoreGive_called = 0;
  xSemaphoreGive_return_value = 1;
  last_taken_semaphore = NULL;
  last_block_time = 0;
  next_return_value = 1;

  memset_called = 0;
  memset_last_ptr = NULL;
  memset_last_val = -1;
  memset_last_size = 0;

  memcpy_called = 0;
  memcpy_dest = NULL;
  memcpy_src = NULL;
  memcpy_size = 0;

  vsnprintf_called = 0;
  vsnprintf_last_buffer[0] = '\0';
  vsnprintf_last_format = NULL;
  vsnprintf_return_value = 0;
}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();

  printf("\n=== SPI_TESTS ===\n");
  RUN_SPI_master_mode0_TESTS();
  RUN_SPI_master_mode1_TESTS();
  RUN_SPI_master_mode2_TESTS();
  RUN_SPI_master_mode3_TESTS();
  printf("\n=== SPI_TESTS completed ===\n");
  printf("\n=== Debugging_TESTS ===\n");
  Debugging_test();
  printf("\n=== Debugging_TESTS completed ===\n");

  return UNITY_END();
}
