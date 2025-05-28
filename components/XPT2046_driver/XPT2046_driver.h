/**
 * @file XPT2046_driver.h
 * @author your name (you@domain.com)
 * @brief this file contains the header for the XPT2046 touch controller driver,
 * which is used to interface with the ILI9488 display and handle touch input.
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef COMPONENTS_XPT2046_DRIVER_XPT2046_DRIVER_H_
#define COMPONENTS_XPT2046_DRIVER_XPT2046_DRIVER_H_

#include "../ILI9488_driver/ILI9488_driver.h"
#include "../my_spiffs/my_spiffs.h"
#include "../tasks/tasks.h"
#include "../I2C/I2C.h"

typedef struct {
	uint16_t x;
	uint16_t y;
	char previous_task[32];
	char current_task[32];
} TaskParams;

extern uint8_t paragraph_number;
extern char full_path[160];
extern volatile bool PIRQ_bool;

uint16_t* recieve_touch_data(int r);
void init_XPT2046();
void send_control_byte(uint8_t parameters);
void draw_IRQ();
void check_key_press(uint16_t x, uint16_t y, uint16_t *x1, uint16_t *y1,
		char *case_type, char *previous_task, char *current_task,
		TaskParams *data);
bool calculate_x_y(uint16_t *x, uint16_t *y) ;

#endif
