#ifndef COMPONENTS_ILI9488_DRIVER_ILI9488_DRIVER_H_
#define COMPONENTS_ILI9488_DRIVER_ILI9488_DRIVER_H_

#include "../SPI/SPI.h"
#include "driver/gpio.h"

#define MOSI 13
#define MISO 12
#define SCK  14
#define SS   15
#define DC_pin 4
#define RESET_pin 2

extern const uint8_t A_font_0[] ICACHE_RODATA_ATTR ;
extern const size_t dataSize ;

void init_display();
void send_ILI9488_data(uint8_t data);
void send_command(uint8_t command);
uint8_t* recieve_data(int r);
void tick_spi_ILI9488();
void set_resolution_pos(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void set_orientation(uint8_t orientation);
void draw_char(char character);

#endif
