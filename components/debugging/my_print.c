#include "my_print.h"

char log_buffer[LOG_BUFFER_SIZE];
static size_t log_index = 0;
static SemaphoreHandle_t log_mutex = NULL;

int my_custom_putchar(int c) {
	// Create a string that contains the character
	char char_str[2] = { (char) c, '\0' }; // Create a string with one character and null-terminator

	// Forward this string to my_print, assuming it works like printf
	my_print("%s", char_str);

	return c;  // Return the character as required by putchar-like functions
}

void my_print_init(void) {

	ESP_ERROR_CHECK(nvs_flash_init());

	esp_log_set_putchar(my_custom_putchar);

	log_mutex = xSemaphoreCreateMutex();
	if (!log_mutex) {
		printf("Failed to create log mutex!\n");
	}
}

// Append a message to the buffer
static void log_to_buffer(const char *msg) {
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

// Your my_print function
void my_print(const char *format, ...) {
	char temp[256];
	va_list args;

	va_start(args, format);
	vsnprintf(temp, sizeof(temp), format, args);
	va_end(args);

	printf("%s", temp);
	log_to_buffer(temp);
}

