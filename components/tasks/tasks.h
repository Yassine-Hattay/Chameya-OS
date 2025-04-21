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

typedef struct {
	uint16_t x;
	uint16_t y;
	char previous_task[32];
	char current_task[32];
} TaskParams;


extern TaskHandle_t main_menu_Handle;
extern TaskHandle_t other_task_handel;
extern char keyboard_buffer[];
extern uint8_t keyboard_buffer_i;

void note_book_task(void *pvParameters);
void bootApp_noteBook();
void note_book_app_page1(void *pvParameters);
void printCoordHistoryTable();
void write_textfile_task(void *pvParameters);
void notebook_editFilesPage1_task(void *pvParameters);

#endif /* COMPONENTS_TASKS_TASKS_H_ */
