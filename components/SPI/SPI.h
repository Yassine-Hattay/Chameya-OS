/*
 * spi.h
 *
 *  Created on: 24 Mar 2025
 *      Author: hatta
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


void spi_slave_init(void) ;
void spi_master_init(void);
uint8_t spi_master_bit_bang_mode_0(uint8_t data_to_send);

#else
#include "SPI_test.h"
#endif


#endif /* COMPONENTS_SPI_SPI_H_ */

