/**
 * @file my_print.c
 * @author your name (you@domain.com)
 * @brief Contains an implemntation of printf that writes to the serial monitor
 * and to a web page through wifi .
 * @version 0.1
 * @date 2025-05-28
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "my_print.h"

char log_buffer[LOG_BUFFER_SIZE];
size_t log_index = 0;
SemaphoreHandle_t log_mutex = NULL;

/**
 * @brief Outputs a single character by forwarding it as a string to my_print.
 *
 * Converts the input character to a null-terminated string and passes it to
 * my_print, mimicking the behavior of putchar for custom output handling.
 */

int my_custom_putchar(int c) {
  // Create a string that contains the character
  char char_str[2] = {
      (char)c, '\0'}; // Create a string with one character and null-terminator

  // Forward this string to my_print, assuming it works like printf
  my_print("%s", char_str);

  return c; // Return the character as required by putchar-like functions
}

/**
 * @brief Initializes the custom print system.
 *
 * Sets up NVS flash, registers a custom putchar function for ESP logging,
 * and creates a mutex to ensure thread-safe logging.
 * This function must be called before using my_print in a multi-threaded
 * environment.
 */

void my_print_init(void) {

  ESP_ERROR_CHECK(nvs_flash_init());

  esp_log_set_putchar(my_custom_putchar);

  log_mutex = xSemaphoreCreateMutex();
  if (!log_mutex)
    printf("Failed to create log mutex!\n");
}

/**
 * @brief Appends a message to the global log buffer in a thread-safe manner.
 *
 * Acquires a mutex to safely append the input message to the log buffer.
 * If the buffer is full, it resets the buffer (simple overflow handling).
 *
 * @param msg The null-terminated message string to append.
 */

void log_to_buffer(const char *msg) {
  if (!log_mutex)
    return;

  xSemaphoreTake(log_mutex, portMAX_DELAY);

  size_t len = strlen(msg);
  if (log_index + len >= LOG_BUFFER_SIZE) {
    // If full, reset (or you can implement ring buffer if you want)
    log_index = 0;
    memset(log_buffer, 0, sizeof(log_buffer));
  }

  memcpy(log_buffer + log_index, msg, len);
  log_index += len;

  xSemaphoreGive(log_mutex);
}

/**
 * @brief Custom print function that outputs formatted text and logs it to a buffer.
 *
 * Formats the input string and arguments like printf, prints the result to the console,
 * and appends it to a global log buffer for later inspection or debugging.
 *
 * @param format The format string (as in printf).
 * @param ...    Additional arguments to format.
 */

void my_print(const char *format, ...) {
  char temp[256];
  va_list args;

  //memcpy(args, format);
  vsnprintf(temp, sizeof(temp), format, args);
  va_end(args);

  printf("%s", temp);
  log_to_buffer(temp);
}
