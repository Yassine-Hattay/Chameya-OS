/*
 * I2C.h
 *
 *  Created on: 27 Mar 2025
 *      Author: hatta
 */

#ifndef COMPONENTS_I2C_I2C_H_
#define COMPONENTS_I2C_I2C_H_

#define BUFFER_SIZE 128

void I2C_init_master();
void I2C_init_salve();
void i2c_start();
void i2c_send_byte(uint8_t byte);
void i2c_stop();
void i2c_write_byte(uint8_t data);
void one_tick();
uint8_t i2c_recive_byte();
void send_ACK_NACK(bool ACK);

extern uint8_t I2C_SDA;
extern uint8_t I2C_SCL;
extern uint8_t I2C_SLAVE_ADDR;

#endif /* COMPONENTS_I2C_I2C_H_ */
