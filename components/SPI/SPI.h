/**
 * @file SPI.h
 * @author your name (you@domain.com)
 * @brief this file implements SPI communication in bit-bang mode for ESP8266.
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef COMPONENTS_SPI_SPI_H_
#define COMPONENTS_SPI_SPI_H_

#include "../my_config/my_config.h"

#if TEST_ON_PC == 0
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"

#define MOSI 13
#define MISO 12
#define SCK  14
#define SS   15

extern uint8_t BMOSI;
extern uint8_t BSS;
extern uint8_t BSCK;

void spi_slave_init(void) ;
void spi_master_init(void);
uint8_t spi_master_bit_bang_mode_0(uint8_t data_to_send);

#else
#include "SPI_test.h"
#endif


#endif /* COMPONENTS_SPI_SPI_H_ */
