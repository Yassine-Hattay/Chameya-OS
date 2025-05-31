/**
 * @file my_print.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef COMPONENTS_DEBUGGING_MY_PRINT_H_
#define COMPONENTS_DEBUGGING_MY_PRINT_H_

#include "../my_config/my_config.h"

void my_print(const char *format, ...);



#if TEST_ON_PC == 0

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>


/** 
 * @brief  Size of the RAM log buffer .
 */

void my_print_init(void);
void start_webserver(void);
extern char log_buffer[];
extern size_t log_index ;
#define LOG_BUFFER_SIZE 4096 // Size of the RAM log buffer

#else
#include "my_print_test.h"
#endif

#endif /* COMPONENTS_DEBUGGING_MY_PRINT_H_ */
