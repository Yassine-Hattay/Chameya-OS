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
extern const uint8_t B_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t C_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t D_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t E_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t F_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t G_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t H_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t I_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t J_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t K_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t L_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t M_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t O_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t P_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t Q_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t R_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t S_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t T_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t U_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t V_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t W_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t X_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t Y_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t Z_font_0[] ICACHE_RODATA_ATTR ;

extern const uint8_t N1_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N2_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N3_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N4_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N5_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N6_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N7_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N8_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N9_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t N0_font_0[] ICACHE_RODATA_ATTR ;

extern const uint8_t __font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t $_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t cent_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t percent_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t comma_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t point_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t exclamation_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t question_mark_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t open_braket_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t close_braket_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t minus_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t plus_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t times_font_0[] ICACHE_RODATA_ATTR ;
extern const uint8_t devide_font_0[] ICACHE_RODATA_ATTR ;


extern size_t letter_font_0_size ;

void init_display();
void send_ILI9488_data(uint8_t data);
void send_command(uint8_t command);
uint8_t* recieve_data(int r);
void tick_spi_ILI9488();
void set_resolution_pos(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void set_orientation(uint8_t orientation);
void draw_char(char character);
void transform_array(const uint8_t *input, uint8_t *output, int input_size,
		int font_size);

#endif
