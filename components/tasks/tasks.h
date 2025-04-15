/*
 * tasks.h
 *
 *  Created on: 15 Apr 2025
 *      Author: hatta
 */

#ifndef COMPONENTS_TASKS_TASKS_H_
#define COMPONENTS_TASKS_TASKS_H_

#include "../ILI9488_driver/ILI9488_driver.h"
#include "../XPT2046_driver/XPT2046_driver.h"
#include <math.h>


extern TaskHandle_t main_menu_Handle;
void note_book_task(void *pvParameters);

#endif /* COMPONENTS_TASKS_TASKS_H_ */
