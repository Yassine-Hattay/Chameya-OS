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
#include "esp_system.h"

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
void keyboard_task(void *pvParameters);
void pong_game_task(void *pvParameters);  // remove
void memory_monitor_task(void *pvParameters);
void pong_gamePage1_task(void *pvParameters);

#endif /* COMPONENTS_TASKS_TASKS_H_ */
