#ifndef COMPONENTS_ILI9488_DRIVER_ILI9488_DRIVER_H_
#define COMPONENTS_ILI9488_DRIVER_ILI9488_DRIVER_H_

#include "../SPI/SPI.h"
#include "driver/gpio.h"
#include <string.h>

#define MISO_touch 16
#define SCK  14
#define SS_display   15
#define SS_touch   5
#define DC_pin 4
#define RESET_pin 2
#define PIRQ_pin 3
#define MAX_COORDS 254 // or whatever max size you need
#define MAX_COORDS_CHAR 254 // or whatever max size you need
#define APP_NAME_MAX_LEN 32  // Or whatever maximum length you expect

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	int16_t correction;
	char app_name[APP_NAME_MAX_LEN];  // Fixed-size string
} CoordHistory;

typedef struct {
	uint16_t x;       // X position
	uint16_t y;       // Y position
	uint16_t width;   // Width of the key
	uint16_t height;  // Height of the key
	char *label;      // Character(s) for the key (could be a string)
} Key;

extern const uint8_t A_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t B_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t C_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t D_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t E_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t F_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t G_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t H_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t I_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t J_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t K_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t L_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t M_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t O_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t P_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t Q_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t R_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t S_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t T_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t U_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t V_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t W_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t X_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t Y_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t Z_font_0[] ICACHE_RODATA_ATTR;

extern const uint8_t a_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t b_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t c_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t d_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t e_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t f_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t g_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t h_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t i_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t j_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t k_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t l_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t m_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t n_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t o_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t p_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t q_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t r_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t s_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t t_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t u_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t v_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t w_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t x_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t y_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t z_font_0[] ICACHE_RODATA_ATTR;

extern const uint8_t N1_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N2_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N3_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N4_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N5_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N6_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N7_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N8_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N9_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t N0_font_0[] ICACHE_RODATA_ATTR;

extern const uint8_t __font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t $_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t percent_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t comma_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t point_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t two_points_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t exclamation_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t question_mark_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t open_braket_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t close_braket_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t minus_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t plus_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t times_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t devide_font_0[] ICACHE_RODATA_ATTR;
extern const uint8_t shift_font_0[] ICACHE_RODATA_ATTR;

extern const uint8_t button[] ICACHE_RODATA_ATTR;

extern uint8_t chameya[] ICACHE_RODATA_ATTR;
extern uint8_t notebook[] ICACHE_RODATA_ATTR;
extern uint8_t coord_index_char;
extern uint8_t coord_index;

extern size_t size_var;
extern size_t size_var1;

extern const size_t letter_font_0_size;
extern char *background_color;

extern CoordHistory history_char[];
extern CoordHistory history[];
extern Key keyboard[];

void init_display();
void send_ILI9488_data(uint8_t data);
void send_command(uint8_t command);
uint8_t* recieve_data(int r);

void tick_spi();

void set_resolution_pos(const uint16_t x, const uint16_t y, uint16_t width,
		uint16_t height, int16_t correction_value);

void set_orientation(uint8_t orientation);
void draw_char(char character);

void transform_array(const uint8_t *input, uint8_t *output, int input_size,
		int font_size);

uint8_t* apply_font_size(const uint8_t *font_data,
		size_t *max_output_size_final, uint8_t font_size);

void print_ILI9488(char *message, uint16_t x, uint16_t y, uint8_t font_size);

void FillScreenBlue();
void FillScreenblack();

uint8_t recive();
void hardware_reset();
void send_data(uint8_t data_to_send);
void clean_screen();
void print_char_ILI9488(char c, uint16_t *x, uint16_t *y, uint8_t font_size);
void clean_last_char();
void print_history(void);
void main_menu_task(void *pvParameters);
void draw_keyborad(char c);
void IRAM_ATTR PIRQ_isr_handler(void *arg);
void draw_main_menu_icons();
void draw_main_menu_icons();
void make_button(char *name, uint16_t width, uint16_t height, uint16_t x,
		uint16_t y);
void bootApp_noteBook();
void make_X_button();
void apply_x_correction(char *message, uint16_t *x, uint16_t *y,
		uint8_t font_size);


#endif
