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
#include "limits.h"
#include "../UART/UART.h"

#define NUM_GOBLINS 10

// ---- Goblin struct ----
typedef struct {
	int xp; // x-position
	int yp; // y-position
	int real_xp;
	int real_yp;
	char orientation;
	int health;
	char name[8];
	uint8_t last_draw_coord_index;
} goblin_torch;

typedef struct {
	goblin_torch *goblins[NUM_GOBLINS];
} goblin_group_t;

typedef struct {
	char orientation;
} map;

extern TaskHandle_t main_menu_Handle;
extern TaskHandle_t other_task_handel;
extern char keyboard_buffer[];
extern uint8_t keyboard_buffer_i;
extern map myMap; // remove
extern goblin_group_t *global_group; //remove
extern bool Read_write_bit_i2c;

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
void goblin_task(void *params); // remove
void GPIO_C_page_1(void *pvParameters);
void GPIO_C_UART(void *pvParameters);
void GPIO_C_UART_Transmit(void *pvParameters);
void GPIO_C_UART_page_0(void *pvParameters);
void GPIO_C_page_1(void *pvParameters) ;

#endif /* COMPONENTS_TASKS_TASKS_H_ */
