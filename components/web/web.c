/**
 * @file web.c
 * @author your name (you@domain.com)
 * @brief this file contains the implementation of the web server for
 * displaying logs from the ESP8266.
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "web.h"

extern void get_logs(char *out_buffer, size_t max_len);

/* FreeRTOS event group to signal when we are connected */
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "wifi station";
static int s_retry_num = 0;

/**
 * @brief Event handler for Wi-Fi and IP events.
 *
 * This function acts as a central handler for various Wi-Fi and IP-related events
 * from the ESP-IDF event loop. It manages Wi-Fi connection attempts, retries,
 * and signals connection status using an FreeRTOS event group.
 * Upon successfully obtaining an IP address, it also initiates a web server.
 *
 * @param arg Arguments passed to the handler (unused in this context).
 * @param event_base The base of the event (e.g., `WIFI_EVENT`, `IP_EVENT`).
 * @param event_id The ID of the specific event (e.g., `WIFI_EVENT_STA_START`, `IP_EVENT_STA_GOT_IP`).
 * @param event_data Pointer to the event-specific data.
 */

static void event_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) {
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT
			&& event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < 3) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG, "connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

		/* Start the web server when we get IP */
		start_webserver();
	}
}

/**
 * @brief Initializes the ESP32 as a Wi-Fi station and connects to an access point.
 *
 * This function sets up the Wi-Fi in station mode, registers event handlers for
 * connection management, attempts to connect to a specified Wi-Fi network,
 * and waits for the connection outcome. It logs success or failure and then
 * unregisters the event handlers.
 */

void wifi_init_sta(void) {
	s_wifi_event_group = xEventGroupCreate();

	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(
			esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(
			esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	wifi_config_t wifi_config = { .sta = { .ssid = EXAMPLE_ESP_WIFI_SSID,
			.password = EXAMPLE_ESP_WIFI_PASS }, };

	if (strlen((char*) wifi_config.sta.password)) {
		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_sta finished.");

	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
	WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
	pdFALSE,
	pdFALSE,
	portMAX_DELAY);

	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
				EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
				EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}

	ESP_ERROR_CHECK(
			esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
					&event_handler));
	ESP_ERROR_CHECK(
			esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
	vEventGroupDelete(s_wifi_event_group);
}

/**
 * @brief HTTP GET handler for serving debug logs.
 *
 * This function handles incoming HTTP GET requests for a debug endpoint.
 * It dynamically allocates a buffer, populates it with system logs obtained
 * from `get_logs()`, sets the response content type to plain text, sends
 * the logs as the HTTP response, and then frees the allocated memory.
 *
 * @param req Pointer to the HTTP request structure.
 * @return `ESP_OK` if the request is handled successfully, `ESP_FAIL` otherwise.
 */

static esp_err_t debug_get_handler(httpd_req_t *req) {
	char *response = malloc(4096);
	if (!response) {
		httpd_resp_send_500(req);
		return ESP_FAIL;
	}

	memset(response, 0, 4096);
	get_logs(response, 4096);

	httpd_resp_set_type(req, "text/plain");
	httpd_resp_send(req, response, strlen(response));

	free(response);
	return ESP_OK;
}
/**
 * @brief HTTP GET handler for serving a dynamic HTML log page.
 *
 * This function processes HTTP GET requests to display system logs in a web browser.
 * It constructs a complete HTML page that includes embedded CSS for styling and
 * JavaScript for auto-reloading and scrolling to the bottom of the logs.
 * The current logs are retrieved from `log_buffer` and embedded directly into the HTML.
 *
 * @param req Pointer to the HTTP request structure.
 * @return `ESP_OK` if the HTML page is successfully sent.
 */
static esp_err_t log_page_get_handler(httpd_req_t *req) {
	// Build the response HTML with current logs
	char html_response[2048];
	snprintf(html_response, sizeof(html_response),
			"<!DOCTYPE html>"
					"<html lang=\"en\">"
					"<head>"
					"<meta charset=\"UTF-8\">"
					"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
					"<title>ESP8266 Log</title>"
					"<style>"
					"body {"
					"    background-color: black;"
					"    color: white;"
					"    font-family: Arial, sans-serif;"
					"    margin: 0;"
					"    padding: 0;"
					"}"
					"pre {"
					"    font-size: 16px;"
					"    white-space: pre-wrap;"
					"    overflow-y: auto;"
					"    max-height: 80vh;"
					"    padding: 10px;"
					"}"
					"</style>"
					"<script>"
					"function scrollToBottom() {"
					"    var pre = document.querySelector('pre');"
					"    pre.scrollTop = pre.scrollHeight;"
					"}"
					"setInterval(function() { location.reload(); }, 500);"
					"window.onload = function() {"
					"    scrollToBottom();"
					"};"
					"</script>"
					"</head>"
					"<body>"
					"<h1>ESP8266 Logs</h1>"
					"<pre id=\"logContent\">%s</pre>"
					"</body>"
					"</html>", log_buffer);

	// Send the HTML response with strlen for the response length
	httpd_resp_send(req, html_response, strlen(html_response));
	return ESP_OK;
}

/**
 * @brief Starts an HTTP web server and registers a handler for the "/logs" URI.
 *
 * This function initializes and starts an embedded HTTP server on the ESP32.
 * It configures the server with default settings and then registers a specific
 * URI handler (`log_page_get_handler`) to serve a dynamic log page when
 * clients access the "/logs" endpoint.
 */

void start_webserver(void) {
	httpd_handle_t server = NULL;

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Register URI handler for the log page
	httpd_uri_t log_uri = { .uri = "/logs", .method = HTTP_GET, .handler =
			log_page_get_handler, .user_ctx = NULL };

	// Start the server
	ESP_ERROR_CHECK(httpd_start(&server, &config));
	ESP_ERROR_CHECK(httpd_register_uri_handler(server, &log_uri));
}
