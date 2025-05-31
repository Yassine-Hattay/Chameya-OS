
#ifndef COMPONENTS_TEST_SPI_TEST_H_
#define COMPONENTS_TEST_SPI_TEST_H_

#include "mocks_fakes.h"

void RUN_SPI_master_mode2_TESTS(void);
void RUN_SPI_master_mode1_TESTS(void);
void RUN_SPI_master_mode3_TESTS(void);
void RUN_SPI_master_mode0_TESTS(void);

void spi_master_init(void);

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char *const pcName,
                       const configSTACK_DEPTH_TYPE usStackDepth,
                       void *const pvParameters, UBaseType_t uxPriority,
                       TaskHandle_t *const pxCreatedTask);

uint8_t spi_master_bit_bang_mode_2(uint8_t data_to_send);
uint8_t spi_master_bit_bang_mode_1(uint8_t data_to_send);
uint8_t spi_master_bit_bang_mode_3(uint8_t data_to_send);
uint8_t spi_master_bit_bang_mode_0(uint8_t data_to_send);

#endif /* COMPONENTS_TEST_SPI_TEST_H_ */
