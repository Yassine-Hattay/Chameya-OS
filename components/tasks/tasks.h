/**
 * @file tasks.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef COMPONENTS_TASKS_TASKS_H_
#define COMPONENTS_TASKS_TASKS_H_

#include "../ILI9488_driver/ILI9488_driver.h"
#include "../XPT2046_driver/XPT2046_driver.h"
#include <math.h>
#include "esp_system.h"
#include "limits.h"
#include "../UART/UART.h"


/** 
 * @brief Maximum number of goblins in the game.
 */

#define NUM_GOBLINS 10

/// @brief min pixel count range so the gobling can reach it's opponent
#define ATTACK_RANGE 20
/// @brief max number of text files .

#define MAX_FILES 100


/** 
 * @brief Represents a goblin character in the game.
 * 
 * This structure holds all necessary state information for a goblin,
 * including its position, orientation, health, name, and data used for rendering.
 */

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



/** 
 * @brief map structure to hold orientation information to draw the goblins in the correct direction.
 */

typedef struct {
	char orientation;
} map;

extern TaskHandle_t main_menu_Handle;
extern TaskHandle_t other_task_handel;

/** 
 * @brief this buffer is used to store the keyboard input.
 */

extern char keyboard_buffer[];

/** 
 * @brief index of the current buffer input .
 */

extern uint8_t keyboard_buffer_i;
extern map myMap; // remove
extern goblin_group_t *global_group; //remove

/** 
 * @brief bool to indicate if we want to read or write to the I2C bus.
 */

extern bool Read_write_bit_i2c;

void note_book_task(void *pvParameters);
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
void GPIO_C_SPI_page_0(void *pvParameters);
#endif /* COMPONENTS_TASKS_TASKS_H_ */
