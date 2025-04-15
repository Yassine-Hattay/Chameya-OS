/*
 * XPT2046_driver.h
 *
 *  Created on: 11 Apr 2025
 *      Author: hatta
 */

#ifndef COMPONENTS_XPT2046_DRIVER_XPT2046_DRIVER_H_
#define COMPONENTS_XPT2046_DRIVER_XPT2046_DRIVER_H_

#include "../ILI9488_driver/ILI9488_driver.h"
#include "../tasks/tasks.h"

extern volatile bool PIRQ_bool ;

uint16_t* recieve_touch_data(int r) ;
void init_XPT2046();
void send_control_byte(uint8_t parameters);
void draw_IRQ();
void check_key_press(uint16_t x, uint16_t y, uint16_t *x1, uint16_t *y1,
		char *case_type) ;

#endif
