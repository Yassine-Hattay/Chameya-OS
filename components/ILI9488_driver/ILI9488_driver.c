/**
 * @file ILI9488_driver.c
 * @author your name (you@domain.com)
 * @brief this is my implementation of the ILI9488 driver
 * @version 0.1
 * @date 2025-05-28
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ILI9488_driver.h"

/** @brief This changes the background color of text possible values (black ,
 * blue , green , red)*/

char *background_color = "black";

uint8_t received[];

/** @brief This contatins the current keyborad keys*/

Key keyboard[32];

/** @brief This is used to store CoordHistory of what is being drawn on the
 * screen*/

CoordHistory history[MAX_COORDS];

/** @brief The index of the last thing that was drawn on the screen */

uint16_t coord_index = 1;

/** @brief This is used to store CoordHistory of the chars that are drawn on the
 * screen*/

CoordHistory history_char[MAX_COORDS_CHAR];

/** @brief The index of the last char that was drawn on the screen */

uint8_t coord_index_char = 1;

/**
 * @brief Fill a rectangle area on the display with a specified color.
 *
 * Sets the drawing region and sends color data to fill the rectangle.
 *
 * @param x Top-left X coordinate of the rectangle.
 * @param y Top-left Y coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param color_byte Color value to fill the rectangle with.
 */

void fill_rect(int x, int y, int width, int height, uint8_t color_byte) {

  set_resolution_pos(x, y, width, height, 0);

  send_command(0x2C);
  int total_pixels = width * height;
  for (int i = 0; i < total_pixels / 2; i++)
    send_ILI9488_data(color_byte);
  send_command(0x00);
}

/**
 * @brief Receives a byte over SPI using software bit-banging.
 * 
 * This function manually toggles the clock line (SCK) to read 8 bits 
 * serially from the MISO_touch line, assembling them into a single byte.
 * It sets the clock low before starting, then for each bit:
 * raises the clock, reads the bit, then lowers the clock.
 * 
 * @return The 8-bit data byte received from the slave device.
 */

uint8_t recive() {
  uint8_t received = 0;
  gpio_set_level(SCK, 0);

  for (int i = 7; i >= 0; i--) {

    gpio_set_level(SCK, 1);
    received |= (gpio_get_level(MISO_touch) << i);
    gpio_set_level(SCK, 0);
  }

  // Return received data
  return received;
}

/**
 * @brief Sends a byte over SPI using software bit-banging.
 * 
 * This function manually toggles the clock line (SCK) to transmit 8 bits 
 * serially through the MOSI line. It sets the clock low before starting,
 * then for each bit: sets MOSI to the bit value, raises the clock, then lowers it.
 * 
 * @param data_to_send The 8-bit data byte to send to the slave device.
 */

void send_data(uint8_t data_to_send) {

  gpio_set_level(SCK, 0);

  for (int i = 7; i >= 0; i--) {
    // Set MOSI
    gpio_set_level(MOSI, (data_to_send >> i) & 1);
    gpio_set_level(SCK, 1);
    gpio_set_level(SCK, 0);
  }
}

/**
 * @brief Generates a single SPI clock pulse by toggling the SCK line.
 * 
 * This function sets the clock line (SCK) low then immediately high,
 * creating one full rising edge used to synchronize SPI data transfer.
 */


void tick_spi() {
  gpio_set_level(SCK, 0);
  gpio_set_level(SCK, 1);
}

/**
 * @brief Performs a hardware reset sequence on the display using the RESET pin.
 * 
 * This function toggles the RESET pin: sets it high, waits 15 ticks,
 * sets it low, waits another 15 ticks, and then sets it high again,
 * effectively resetting the connected hardware.
 */

void hardware_reset() {

  gpio_set_level(RESET_pin, 1);
  vTaskDelay(15);
  gpio_set_level(RESET_pin, 0);
  vTaskDelay(15);
  gpio_set_level(RESET_pin, 1);
}

/**
 * @brief Sends a command byte to the device by setting the DC pin low before transmission.
 * 
 * If the DC (Data/Command) pin is currently high (data mode), it is set low (command mode)
 * to indicate that the following byte is a command, then the command byte is sent.
 */

void send_command(uint8_t command) {
  if (gpio_get_level(DC_pin))
    gpio_set_level(DC_pin, 0);
  send_data(command);
}

/**
 * @brief Sends a data byte to the ILI9488 display by setting the DC pin high before transmission.
 * 
 * If the DC (Data/Command) pin is currently low (command mode), it is set high (data mode)
 * to indicate that the following byte is display data, then the data byte is sent.
 */

void send_ILI9488_data(uint8_t data) {
  if (!gpio_get_level(DC_pin))
    gpio_set_level(DC_pin, 1);
  send_data(data);
}


/**
 * @brief Initializes the display hardware and prepares it for operation.
 * 
 * Configures GPIO pins for the display's SS, RESET, and DC lines as outputs,
 * performs a hardware reset, initializes SPI communication, and sends the
 * necessary commands to wake the display from sleep mode and enable normal display.
 */

void init_display() {
  gpio_config_t io_conf = {.pin_bit_mask =
                               (1ULL << SS_display) | (1ULL << RESET_pin),
                           .mode = GPIO_MODE_OUTPUT,
                           .pull_up_en = GPIO_PULLUP_DISABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);

  gpio_config_t io_conf1 = {.pin_bit_mask = (1ULL << DC_pin),
                            .mode = GPIO_MODE_OUTPUT,
                            .pull_up_en = GPIO_PULLUP_ENABLE,
                            .pull_down_en = GPIO_PULLDOWN_DISABLE,
                            .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf1);

  hardware_reset();

  spi_master_init();

  gpio_set_level(SS_display, 0);
  gpio_set_level(DC_pin, 0);

  send_command(0x11); // Sleep OUT
  send_command(0x13); //  Normal Display Mode ON
  send_command(0x29); // Display ON
}

/**
 * @brief Receives a sequence of bytes from the SPI interface.
 * 
 * Reads 'r' bytes by calling the `recive()` function repeatedly, stores
 * them in a global buffer `received`, and prints each byte in binary format.
 * Ensures the DC pin is set high before receiving data.
 * 
 * @param r Number of bytes to receive; must be greater than zero.
 * @return Pointer to the buffer containing the received bytes, or NULL if invalid 'r'.
 */

uint8_t *recieve_data(int r) {

  if (r <= 0) {
    printf("Invalid value for r\n");
    return NULL;
  }

  if (!gpio_get_level(DC_pin))
    gpio_set_level(DC_pin, 1);

  for (int i = 0; i < r; i++)
    received[i] = recive(); // You must have a `recive()` function

  for (int j = 0; j < r; j++) {
    printf("received%d: ", j + 1);
    for (int i = 7; i >= 0; i--)
      printf("%d", (received[j] >> i) & 1);
    printf("\n");
  }

  return received;
}

/**
 * @brief Sets the display area for subsequent pixel data and records coordinates.
 * 
 * Configures the display's active window by specifying the top-left corner (x, y)
 * and the width and height of the rectangular area to update. Saves these parameters
 * with an optional correction value in a history buffer if within limits.
 * Sends the necessary commands to set the column and page address ranges on the
 * ILI9488 display controller.
 * 
 * @param x Top-left X coordinate of the area.
 * @param y Top-left Y coordinate of the area.
 * @param width Width of the area in pixels.
 * @param height Height of the area in pixels.
 * @param correction_value Optional correction value related to coordinate history.
 */

void set_resolution_pos(const uint16_t x, const uint16_t y, uint16_t width,
                        uint16_t height, int16_t correction_value) {

  if (coord_index < MAX_COORDS && (width != 480 && height != 320)) {
    history[coord_index].x = x;
    history[coord_index].y = y;
    history[coord_index].width = width;
    history[coord_index].height = height;
    history[coord_index].correction = correction_value;
    coord_index++;
  } else if (width != 480 && height != 320) {
    coord_index = 1;
  }

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x06);

  uint16_t x_end =
      x + width - 1; // Dereference x to get the current value and modify it
  uint16_t y_end =
      y + height; // Dereference y to get the current value and modify it

  // Split into high and low bytes
  uint8_t x_start_high = (x >> 8) & 0xFF;
  uint8_t x_start_low = x & 0xFF;
  uint8_t x_end_high = (x_end >> 8) & 0xFF;
  uint8_t x_end_low = x_end & 0xFF;

  uint8_t y_start_high = (y >> 8) & 0xFF;
  uint8_t y_start_low = y & 0xFF;
  uint8_t y_end_high = (y_end >> 8) & 0xFF;
  uint8_t y_end_low = y_end & 0xFF;

  send_command(0x2A); // Column Address Set
  send_ILI9488_data(x_start_high);
  send_ILI9488_data(x_start_low);
  send_ILI9488_data(x_end_high);
  send_ILI9488_data(x_end_low);

  send_command(0x2B); // Page Address Set
  send_ILI9488_data(y_start_high);
  send_ILI9488_data(y_start_low);
  send_ILI9488_data(y_end_high);
  send_ILI9488_data(y_end_low);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x01);
}

/**
 * @brief Sets the display area for character rendering and logs the coordinates.
 * 
 * Defines the rectangular region on the display where characters will be drawn,
 * specified by the top-left corner (x, y) and the width and height. Stores these
 * parameters with an optional correction value in a character-specific history buffer.
 * Sends commands to the ILI9488 controller to set column and page address ranges accordingly.
 * 
 * @param x Top-left X coordinate of the character area.
 * @param y Top-left Y coordinate of the character area.
 * @param width Width of the character area in pixels.
 * @param height Height of the character area in pixels.
 * @param correction_value Optional correction value for coordinate history.
 */


static void set_resolution_pos_char(const uint16_t x, const uint16_t y,
                                    uint16_t width, uint16_t height,
                                    int16_t correction_value) {

  if (coord_index_char < MAX_COORDS_CHAR) {
    history_char[coord_index_char].x = x;
    history_char[coord_index_char].y = y;
    history_char[coord_index_char].width = width;
    history_char[coord_index_char].height = height;
    history_char[coord_index_char].correction = correction_value;
    coord_index_char++;
  }

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x06);

  uint16_t x_end =
      x + width - 1; // Dereference x to get the current value and modify it
  uint16_t y_end =
      y + height; // Dereference y to get the current value and modify it

  // Split into high and low bytes
  uint8_t x_start_high = (x >> 8) & 0xFF;
  uint8_t x_start_low = x & 0xFF;
  uint8_t x_end_high = (x_end >> 8) & 0xFF;
  uint8_t x_end_low = x_end & 0xFF;

  uint8_t y_start_high = (y >> 8) & 0xFF;
  uint8_t y_start_low = y & 0xFF;
  uint8_t y_end_high = (y_end >> 8) & 0xFF;
  uint8_t y_end_low = y_end & 0xFF;

  send_command(0x2A); // Column Address Set
  send_ILI9488_data(x_start_high);
  send_ILI9488_data(x_start_low);
  send_ILI9488_data(x_end_high);
  send_ILI9488_data(x_end_low);

  send_command(0x2B); // Page Address Set
  send_ILI9488_data(y_start_high);
  send_ILI9488_data(y_start_low);
  send_ILI9488_data(y_end_high);
  send_ILI9488_data(y_end_low);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x01);
}

/**
 * @brief Sets the display orientation by configuring the ILI9488 MADCTL register.
 * 
 * Sends the command to modify the Memory Access Control register (0x36) of the
 * ILI9488 display controller. Depending on the orientation parameter (0-7),
 * it sets the corresponding data byte to rotate or mirror the display output.
 * If the provided orientation is outside the valid range [0-7], an error message
 * is printed.
 * 
 * @param orientation Orientation index (0 to 7) selecting the display rotation/mirroring.
 */

void set_orientation(uint8_t orientation) {

  send_command(0x36);

  if (orientation == 0)
    send_ILI9488_data(0x08);
  else if (orientation == 1)
    send_ILI9488_data(0x28);
  else if (orientation == 2)
    send_ILI9488_data(0x48);
  else if (orientation == 3)
    send_ILI9488_data(0x68);
  else if (orientation == 4)
    send_ILI9488_data(0x88);
  else if (orientation == 5)
    send_ILI9488_data(0xA8);
  else if (orientation == 6)
    send_ILI9488_data(0xC8);
  else if (orientation == 7)
    send_ILI9488_data(0xE8);
  else
    printf("Error : orientation must be between [0,7] ");
}

/**
 * @brief Transforms an input byte array into an output array with specific byte mappings and duplication.
 * 
 * For each byte in the input array, maps it to two bytes in the output according to predefined rules:
 * - 0x00 maps to 0x00, 0x00
 * - 0xFF maps to 0xFF, 0xFF
 * - 0xF8 maps to 0xFF, 0x00
 * - 0x07 maps to 0x00, 0xFF
 * 
 * After processing every (3 * font_size) bytes from the input, duplicates the last
 * (3 * 2 * font_size) bytes in the output immediately following them, extending the output size.
 * 
 * @param input Pointer to the input byte array.
 * @param output Pointer to the output byte array (must have sufficient space).
 * @param input_size Number of bytes in the input array.
 * @param font_size Parameter influencing chunk size and duplication length.
 */

void transform_array(const uint8_t *input, uint8_t *output, int input_size,
                     int font_size) {
  size_t output_idx = 0;
  for (size_t i = 0; i < input_size; i++) {

    if (input[i] == 0x00) {
      output[output_idx++] = 0x00;
      output[output_idx++] = 0x00;
    } else if (input[i] == 0xFF) {
      output[output_idx++] = 0xFF;
      output[output_idx++] = 0xFF;
    } else if (input[i] == 0xF8) {
      output[output_idx++] = 0xFF;
      output[output_idx++] = 0x00;
    } else if (input[i] == 0x07) {
      output[output_idx++] = 0x00;
      output[output_idx++] = 0xFF;
    }

    if ((i + 1) % (3 * font_size) == 0) {
      int j = 0;

      for (int index = output_idx - (3 * 2 * font_size);
           index <= output_idx - 1; index++) {
        output[output_idx + j] = output[index];
        j = j + 1;
      }
      output_idx = output_idx + 6 * font_size;
    }
  }
}

/**
 * @brief Applies scaling transformations to font data according to the specified font size.
 * 
 * Allocates and returns a new buffer containing the scaled font data.
 * Handles font sizes from 1 to 5 with recursive transformations.
 * Returns NULL if memory allocation fails or font size is invalid.
 * 
 * @param font_data Pointer to the original font data array.
 * @param max_output_size_final Pointer to a size_t variable to receive the output buffer size.
 * @param font_size Desired scaling factor (1 to 5).
 * @return Pointer to the newly allocated scaled font data buffer; must be freed by the caller.
 */


uint8_t *apply_font_size(const uint8_t *font_data,
                         size_t *max_output_size_final, uint8_t font_size) {

  if (font_size == 1) {
    *max_output_size_final = letter_font_0_size * 4;
    uint8_t *output = (uint8_t *)malloc(*max_output_size_final + 2);
    if (!output) {
      printf("Memory allocation failed for output.\n");
      return NULL;
    }
    transform_array(font_data, output, letter_font_0_size, 1);
    return output;
  } else if (font_size >= 2 && font_size <= 5) {
    uint8_t **buffers = (uint8_t **)malloc(font_size * sizeof(uint8_t *));
    size_t *sizes = (size_t *)malloc(font_size * sizeof(size_t));

    if (!buffers || !sizes) {
      printf("Memory allocation failed for buffers/sizes.\n");
      free(buffers);
      free(sizes);
      return NULL;
    }

    sizes[0] = letter_font_0_size * 4;
    for (int i = 1; i < font_size; ++i)
      sizes[i] = sizes[i - 1] * 4;
    *max_output_size_final = sizes[font_size - 1];

    for (int i = 0; i < font_size; ++i) {
      if (i == font_size - 1)
        buffers[i] = (uint8_t *)malloc(sizes[i] + 2);
      else
        buffers[i] = (uint8_t *)malloc(sizes[i]);
      if (!buffers[i]) {
        printf("Memory allocation failed for output.\n");
        for (int j = 0; j < i; ++j)
          free(buffers[j]);
        free(buffers);
        free(sizes);
        return NULL;
      }
    }

    transform_array(font_data, buffers[0], letter_font_0_size, 1);
    for (int i = 1; i < font_size; ++i) {
      transform_array(buffers[i - 1], buffers[i], sizes[i - 1],
                      2 * (1 << (i - 1)));
      free(buffers[i - 1]);
    }

    uint8_t *result = buffers[font_size - 1];
    free(buffers);
    free(sizes);
    return result;
  }
  return NULL;
}

/**
 * @brief Renders a character on the screen with specified font size and color correction.
 * 
 * Applies font scaling, sets drawing position, adjusts colors based on background,
 * and sends the pixel data to the display controller.
 * Updates the current drawing position for the next character.
 * 
 * @param a_font_0 Pointer to the base font data for the character.
 * @param width Width of the character in pixels.
 * @param height Height of the character in pixels.
 * @param x Pointer to the current X coordinate; updated after drawing.
 * @param y Pointer to the current Y coordinate; updated if line wraps.
 * @param font_size Font scaling factor.
 * @param correction_value Horizontal correction to adjust character spacing.
 */

static void print_to_screen_char(const uint8_t a_font_0[], uint16_t width,
                                 uint16_t height, uint16_t *x, uint16_t *y,
                                 uint8_t font_size, int16_t correction_value) {

  uint8_t *output_data = NULL; // Initialize to NULL
  size_t output_size;

  if ((*x) + width > 480 || ((*y) == 0 && ((*x) + width > 456))) {
    (*x) = 0;
    (*y) = (*y) + height;
  }
  if ((*y) + height > 174) {
    free(output_data);
    return;
  }

  output_data = apply_font_size(a_font_0, &output_size, font_size);

  set_resolution_pos_char(*x, *y, width, height, correction_value);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x01);

  if (strcmp(background_color, "red") == 0) {
    for (uint64_t i = 0; i < output_size; i++)
      if (output_data[i] == 0x00)
        output_data[i] = 0x24;
      else if (output_data[i] == 0x07)
        output_data[i] = 0x2F;
      else if (output_data[i] == 0xF8)
        output_data[i] = 0xFC;

    for (uint64_t i = output_size; i < output_size + 2; i++)
      output_data[i] = 0x24;
  } else if (strcmp(background_color, "blue") == 0) {
    for (uint64_t i = 0; i < output_size; i++)
      if (output_data[i] == 0x00)
        output_data[i] = 0x09;
      else if (output_data[i] == 0x07)
        output_data[i] = 0x0F;
      else if (output_data[i] == 0xF8)
        output_data[i] = 0xF9;

    for (uint64_t i = output_size; i < output_size + 2; i++)
      output_data[i] = 0x09;
  } else if (strcmp(background_color, "black") == 0) {
    for (uint64_t i = output_size; i < output_size + 2; i++)
      output_data[i] = 0x00;
  }

  send_command(0x2C);

  for (uint64_t i = 0; i < output_size + 2; i++)
    send_ILI9488_data(output_data[i]);

  (*x) = (*x) + width - correction_value;

  free(output_data);
}

/**
 * @brief Renders a scaled character or graphic block on the screen with color adjustments.
 * 
 * Manages line wrapping, applies font size scaling, sets display area,
 * modifies pixel colors according to the current background color,
 * and sends pixel data to the display controller.
 * Updates the current drawing coordinates for subsequent rendering.
 * 
 * @param a_font_0 Pointer to the base font or image data.
 * @param width Width of the character/block in pixels.
 * @param height Height of the character/block in pixels.
 * @param x Pointer to the current X coordinate; updated after rendering.
 * @param y Pointer to the current Y coordinate; updated after line wrap.
 * @param font_size Scaling factor for font size.
 * @param correction_value Horizontal adjustment to spacing after rendering.
 */

static void print_to_screen(const uint8_t a_font_0[], uint16_t width,
                            uint16_t height, uint16_t *x, uint16_t *y,
                            uint8_t font_size, int16_t correction_value) {

  uint8_t *output_data;
  size_t output_size;

  if ((*x) + width > 480) {
    (*x) = 0;
    (*y) = (*y) + height;
  }

  output_data = apply_font_size(a_font_0, &output_size, font_size);

  set_resolution_pos(*x, *y, width, height, correction_value);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x01);

  if (strcmp(background_color, "red") == 0) {
    for (uint64_t i = 0; i < output_size; i++)
      if (output_data[i] == 0x00)
        output_data[i] = 0x24;
      else if (output_data[i] == 0x07)
        output_data[i] = 0x2F;
      else if (output_data[i] == 0xF8)
        output_data[i] = 0xFC;

    for (uint64_t i = output_size; i < output_size + 2; i++)
      output_data[i] = 0x24;
  } else if (strcmp(background_color, "blue") == 0) {
    for (uint64_t i = 0; i < output_size; i++)
      if (output_data[i] == 0x00)
        output_data[i] = 0x09;
      else if (output_data[i] == 0x07)
        output_data[i] = 0x0F;
      else if (output_data[i] == 0xF8)
        output_data[i] = 0xF9;

    for (uint64_t i = output_size; i < output_size + 2; i++)
      output_data[i] = 0x09;
  } else if (strcmp(background_color, "black") == 0) {
    for (uint64_t i = output_size; i < output_size + 2; i++)
      output_data[i] = 0x00;
  } else if (strcmp(background_color, "green") == 0) {
    for (uint64_t i = 0; i < output_size; i++)
      if (output_data[i] == 0x00)
        output_data[i] = 0x12;
      else if (output_data[i] == 0x07)
        output_data[i] = 0x17;
      else if (output_data[i] == 0xF8)
        output_data[i] = 0xFA;

    for (uint64_t i = output_size; i < output_size + 2; i++)
      output_data[i] = 0x12;
  }

  send_command(0x2C);

  for (uint64_t i = 0; i < output_size + 2; i++)
    send_ILI9488_data(output_data[i]);

  (*x) = (*x) + width - correction_value;

  free(output_data);
}

/**
 * @brief Adjusts drawing coordinates with line wrapping and horizontal correction.
 * 
 * Checks if the next character exceeds screen width and moves to a new line if needed.
 * Then updates the X coordinate by adding the width minus a correction value.
 * 
 * @param a_font_0 Pointer to the base font data (unused in this function).
 * @param width Width of the character/block in pixels.
 * @param height Height of the character/block in pixels.
 * @param x Pointer to the current X coordinate; updated after correction.
 * @param y Pointer to the current Y coordinate; updated if line wraps.
 * @param font_size Font scaling factor (unused in this function).
 * @param correction_value Horizontal spacing adjustment after rendering.
 */

static void inner_apply_correction(const uint8_t a_font_0[], uint16_t width,
                                   uint16_t height, uint16_t *x, uint16_t *y,
                                   uint8_t font_size,
                                   int16_t correction_value) {

  if ((*x) + width > 480) {
    (*x) = 0;
    (*y) = (*y) + height;
  }

  (*x) = (*x) + width - correction_value;
}

/**
 * @brief Applies horizontal spacing corrections to each character in a message string.
 * 
 * Iterates through the input message, adjusting the X and Y drawing coordinates based on
 * the font size and specific character spacing corrections. Handles spaces by advancing
 * the X position by the character width, and calls an internal function with character-specific
 * font data and correction factors for all other supported characters.
 * 
 * @param message Pointer to the null-terminated string to process.
 * @param x Pointer to the current X coordinate; updated as characters are processed.
 * @param y Pointer to the current Y coordinate; may be updated by the internal correction function.
 * @param font_size Integer representing the font size (0 to 5) which determines character dimensions.
 */

void apply_x_correction(char *message, uint16_t *x, uint16_t *y,
                        uint8_t font_size) {

  uint16_t width;
  uint16_t height;

  switch (font_size) {
  case 0:
    width = 6;
    height = 7;
    break;
  case 1:
    width = 12;
    height = 15;
    break;
  case 2:
    width = 24;
    height = 29;
    break;
  case 3:
    width = 48;
    height = 57;
    break;
  case 4: // same values for 3 and 4
    width = 96;
    height = 113;
    break;
  case 5: // same values for 3 and 4
    width = 192;
    height = 225;
    break;
  default:
    printf("error select a font size must be between 0 and 5 ! \n");
    return;
  }

  while (*message) {

    if (*message == ' ') {
      {
        *x += width; // Move the x position forward by the width of a character
      }
    } else if (*message == 'a') {
      inner_apply_correction(a_font_0, width, height, x, y, font_size,
                             width * 0.2);
    } else if (*message == 'b') {
      inner_apply_correction(b_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'c') {
      inner_apply_correction(c_font_0, width, height, x, y, font_size,
                             width * 0.2);
    } else if (*message == 'd') {
      inner_apply_correction(d_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'e') {
      inner_apply_correction(e_font_0, width, height, x, y, font_size,
                             width * 0.18);
    } else if (*message == 'f') {
      inner_apply_correction(f_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'g') {
      inner_apply_correction(g_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'h') {
      inner_apply_correction(h_font_0, width, height, x, y, font_size,
                             width * 0.2);
    } else if (*message == 'i') {
      inner_apply_correction(i_font_0, width, height, x, y, font_size,
                             width * 0.6);
    } else if (*message == 'j') {
      inner_apply_correction(j_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'k') {
      inner_apply_correction(k_font_0, width, height, x, y, font_size,
                             width * 0.2);
    } else if (*message == 'l') {
      inner_apply_correction(l_font_0, width, height, x, y, font_size,
                             width * 0.6);
    } else if (*message == 'm') {
      inner_apply_correction(m_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'n') {
      inner_apply_correction(n_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'o') {
      inner_apply_correction(o_font_0, width, height, x, y, font_size,
                             width * 0.2);
    } else if (*message == 'p') {
      inner_apply_correction(p_font_0, width, height, x, y, font_size,
                             width * 0.2);
    } else if (*message == 'q') {
      inner_apply_correction(q_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'r') {
      inner_apply_correction(r_font_0, width, height, x, y, font_size,
                             width * 0.25);
    } else if (*message == 's') {
      inner_apply_correction(s_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 't') {
      inner_apply_correction(t_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'u') {
      inner_apply_correction(u_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'v') {
      inner_apply_correction(v_font_0, width, height, x, y, font_size,
                             width * 0.25);
    } else if (*message == 'w') {
      inner_apply_correction(w_font_0, width, height, x, y, font_size, 0);
    } else if (*message == 'x') {
      inner_apply_correction(x_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'y') {
      inner_apply_correction(y_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'z') {
      inner_apply_correction(z_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '0') {
      inner_apply_correction(N0_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '1') {
      inner_apply_correction(N1_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '2') {
      inner_apply_correction(N2_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '3') {
      inner_apply_correction(N3_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '4') {
      inner_apply_correction(N4_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '5') {
      inner_apply_correction(N5_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '6') {
      inner_apply_correction(N6_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '7') {
      inner_apply_correction(N7_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '8') {
      inner_apply_correction(N8_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '9') {
      inner_apply_correction(N9_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '$') {
      inner_apply_correction($_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '%') {
      inner_apply_correction(percent_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == ',') {
      inner_apply_correction(comma_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '.') {
      inner_apply_correction(point_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == ':') {
      inner_apply_correction(two_points_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '!') {
      inner_apply_correction(exclamation_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '?') {
      inner_apply_correction(question_mark_font_0, width, height, x, y,
                             font_size, width * 0.1);
    } else if (*message == '(') {
      inner_apply_correction(open_braket_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == ')') {
      inner_apply_correction(close_braket_font_0, width, height, x, y,
                             font_size, width * 0.1);
    } else if (*message == '-') {
      inner_apply_correction(minus_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '+') {
      inner_apply_correction(plus_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '*') {
      inner_apply_correction(times_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '/') {
      inner_apply_correction(devide_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'A') {
      inner_apply_correction(A_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'B') {
      inner_apply_correction(B_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'C') {
      inner_apply_correction(C_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'D') {
      inner_apply_correction(D_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'E') {
      inner_apply_correction(E_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'F') {
      inner_apply_correction(F_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'G') {
      inner_apply_correction(G_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'H') {
      inner_apply_correction(H_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'I') {
      inner_apply_correction(I_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'J') {
      inner_apply_correction(J_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'K') {
      inner_apply_correction(K_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'L') {
      inner_apply_correction(L_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'M') {
      inner_apply_correction(M_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'N') {
      inner_apply_correction(N_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'O') {
      inner_apply_correction(O_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'P') {
      inner_apply_correction(P_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'Q') {
      inner_apply_correction(Q_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'R') {
      inner_apply_correction(R_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'S') {
      inner_apply_correction(S_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'T') {
      inner_apply_correction(T_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'U') {
      inner_apply_correction(U_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'V') {
      inner_apply_correction(V_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'W') {
      inner_apply_correction(W_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'X') {
      inner_apply_correction(X_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'Y') {
      inner_apply_correction(Y_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == 'Z') {
      inner_apply_correction(Z_font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '_') {
      inner_apply_correction(__font_0, width, height, x, y, font_size,
                             width * 0.1);
    } else if (*message == '~') {
      inner_apply_correction(shift_font_0, width, height, x, y, font_size,
                             width * 0.1);
    }

    message++;
  }
}

/**
 * @brief Prints a given string message on the ILI9488 display at specified coordinates and font size.
 *
 * @param message Pointer to the null-terminated string to be printed.
 * @param x The starting x-coordinate (horizontal position) on the display.
 * @param y The starting y-coordinate (vertical position) on the display.
 * @param font_size Font size index (0 to 5) determining the character dimensions.
 */

void print_ILI9488(char *message, uint16_t x, uint16_t y, uint8_t font_size) {

  uint16_t width;
  uint16_t height;

  switch (font_size) {
  case 0:
    width = 6;
    height = 7;
    break;
  case 1:
    width = 12;
    height = 15;
    break;
  case 2:
    width = 24;
    height = 29;
    break;
  case 3:
    width = 48;
    height = 57;
    break;
  case 4: // same values for 3 and 4
    width = 96;
    height = 113;
    break;
  case 5: // same values for 3 and 4
    width = 192;
    height = 225;
    break;
  default:
    printf("error select a font size must be between 0 and 5 ! \n");
    return;
  }

  while (*message) {

    if (*message == ' ') {
      {
        x += width; // Move the x position forward by the width of a character
      }
    } else if (*message == 'a') {
      print_to_screen(a_font_0, width, height, &x, &y, font_size, width * 0.2);
    } else if (*message == 'b') {
      print_to_screen(b_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'c') {
      print_to_screen(c_font_0, width, height, &x, &y, font_size, width * 0.2);
    } else if (*message == 'd') {
      print_to_screen(d_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'e') {
      print_to_screen(e_font_0, width, height, &x, &y, font_size, width * 0.18);
    } else if (*message == 'f') {
      print_to_screen(f_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'g') {
      print_to_screen(g_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'h') {
      print_to_screen(h_font_0, width, height, &x, &y, font_size, width * 0.2);
    } else if (*message == 'i') {
      print_to_screen(i_font_0, width, height, &x, &y, font_size, width * 0.6);
    } else if (*message == 'j') {
      print_to_screen(j_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'k') {
      print_to_screen(k_font_0, width, height, &x, &y, font_size, width * 0.2);
    } else if (*message == 'l') {
      print_to_screen(l_font_0, width, height, &x, &y, font_size, width * 0.6);
    } else if (*message == 'm') {
      print_to_screen(m_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'n') {
      print_to_screen(n_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'o') {
      print_to_screen(o_font_0, width, height, &x, &y, font_size, width * 0.2);
    } else if (*message == 'p') {
      print_to_screen(p_font_0, width, height, &x, &y, font_size, width * 0.2);
    } else if (*message == 'q') {
      print_to_screen(q_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'r') {
      print_to_screen(r_font_0, width, height, &x, &y, font_size, width * 0.25);
    } else if (*message == 's') {
      print_to_screen(s_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 't') {
      print_to_screen(t_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'u') {
      print_to_screen(u_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'v') {
      print_to_screen(v_font_0, width, height, &x, &y, font_size, width * 0.25);
    } else if (*message == 'w') {
      print_to_screen(w_font_0, width, height, &x, &y, font_size, 0);
    } else if (*message == 'x') {
      print_to_screen(x_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'y') {
      print_to_screen(y_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'z') {
      print_to_screen(z_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '0') {
      print_to_screen(N0_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '1') {
      print_to_screen(N1_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '2') {
      print_to_screen(N2_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '3') {
      print_to_screen(N3_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '4') {
      print_to_screen(N4_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '5') {
      print_to_screen(N5_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '6') {
      print_to_screen(N6_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '7') {
      print_to_screen(N7_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '8') {
      print_to_screen(N8_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '9') {
      print_to_screen(N9_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '$') {
      print_to_screen($_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '%') {
      print_to_screen(percent_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == ',') {
      print_to_screen(comma_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == '.') {
      print_to_screen(point_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == ':') {
      print_to_screen(two_points_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == '!') {
      print_to_screen(exclamation_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == '?') {
      print_to_screen(question_mark_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == '(') {
      print_to_screen(open_braket_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == ')') {
      print_to_screen(close_braket_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == '-') {
      print_to_screen(minus_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == '+') {
      print_to_screen(plus_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == '*') {
      print_to_screen(times_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == '/') {
      print_to_screen(devide_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    } else if (*message == 'A') {
      print_to_screen(A_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'B') {
      print_to_screen(B_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'C') {
      print_to_screen(C_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'D') {
      print_to_screen(D_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'E') {
      print_to_screen(E_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'F') {
      print_to_screen(F_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'G') {
      print_to_screen(G_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'H') {
      print_to_screen(H_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'I') {
      print_to_screen(I_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'J') {
      print_to_screen(J_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'K') {
      print_to_screen(K_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'L') {
      print_to_screen(L_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'M') {
      print_to_screen(M_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'N') {
      print_to_screen(N_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'O') {
      print_to_screen(O_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'P') {
      print_to_screen(P_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'Q') {
      print_to_screen(Q_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'R') {
      print_to_screen(R_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'S') {
      print_to_screen(S_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'T') {
      print_to_screen(T_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'U') {
      print_to_screen(U_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'V') {
      print_to_screen(V_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'W') {
      print_to_screen(W_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'X') {
      print_to_screen(X_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'Y') {
      print_to_screen(Y_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == 'Z') {
      print_to_screen(Z_font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '_') {
      print_to_screen(__font_0, width, height, &x, &y, font_size, width * 0.1);
    } else if (*message == '~') {
      print_to_screen(shift_font_0, width, height, &x, &y, font_size,
                      width * 0.1);
    }

    message++;
  }
}

/**
 * @brief Prints a single character on the ILI9488 display, handling positioning and font size.
 *
 * This function determines the character's pixel width and height based on the selected font size,
 * manages line wrapping for spaces, and renders the appropriate bitmap for the given character.
 *
 * @param c The character to print.
 * @param x Pointer to the current x-coordinate; updated after printing.
 * @param y Pointer to the current y-coordinate; updated if line wrapping occurs.
 * @param font_size Font size index (0 to 5) determining character dimensions.
 */

void print_char_ILI9488(char c, uint16_t *x, uint16_t *y, uint8_t font_size) {

  uint16_t width;
  uint16_t height;

  switch (font_size) {
  case 0:
    width = 6;
    height = 7;
    break;
  case 1:
    width = 12;
    height = 15;
    break;
  case 2:
    width = 24;
    height = 29;
    break;
  case 3:
    width = 48;
    height = 57;
    break;
  case 4: // same values for 3 and 4
    width = 96;
    height = 113;
    break;
  case 5: // same values for 3 and 4
    width = 192;
    height = 225;
    break;
  default:
    printf("error select a font size must be between 0 and 5 ! \n");
    return;
  }

  if (c == ' ') {
    {

      if ((*x) + width > 480 && (*y) < 145) {
        (*x) = 0;
        (*y) = (*y) + height;
      }

      if ((*y) + height > 174)
        return;

      history_char[coord_index_char].x = *x;
      history_char[coord_index_char].y = *y;
      history_char[coord_index_char].width = width;
      history_char[coord_index_char].height = height;
      history_char[coord_index_char].correction = 0;
      coord_index_char++;

      *x += width;
    }
  } else if (c == 'a') {
    print_to_screen_char(a_font_0, width, height, x, y, font_size, width * 0.2);
  } else if (c == 'b') {
    print_to_screen_char(b_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'c') {
    print_to_screen_char(c_font_0, width, height, x, y, font_size, width * 0.2);
  } else if (c == 'd') {
    print_to_screen_char(d_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'e') {
    print_to_screen_char(e_font_0, width, height, x, y, font_size,
                         width * 0.18);
  } else if (c == 'f') {
    print_to_screen_char(f_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'g') {
    print_to_screen_char(g_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'h') {
    print_to_screen_char(h_font_0, width, height, x, y, font_size, width * 0.2);
  } else if (c == 'i') {
    print_to_screen_char(i_font_0, width, height, x, y, font_size, width * 0.6);
  } else if (c == 'j') {
    print_to_screen_char(j_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'k') {
    print_to_screen_char(k_font_0, width, height, x, y, font_size, width * 0.2);
  } else if (c == 'l') {
    print_to_screen_char(l_font_0, width, height, x, y, font_size, width * 0.6);
  } else if (c == 'm') {
    print_to_screen_char(m_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'n') {
    print_to_screen_char(n_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'o') {
    print_to_screen_char(o_font_0, width, height, x, y, font_size, width * 0.2);
  } else if (c == 'p') {
    print_to_screen_char(p_font_0, width, height, x, y, font_size, width * 0.2);
  } else if (c == 'q') {
    print_to_screen_char(q_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'r') {
    print_to_screen_char(r_font_0, width, height, x, y, font_size,
                         width * 0.25);
  } else if (c == 's') {
    print_to_screen_char(s_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 't') {
    print_to_screen_char(t_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'u') {
    print_to_screen_char(u_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'v') {
    print_to_screen_char(v_font_0, width, height, x, y, font_size,
                         width * 0.25);
  } else if (c == 'w') {
    print_to_screen_char(w_font_0, width, height, x, y, font_size, 0);
  } else if (c == 'x') {
    print_to_screen_char(x_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'y') {
    print_to_screen_char(y_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'z') {
    print_to_screen_char(z_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == '0') {
    print_to_screen_char(N0_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '1') {
    print_to_screen_char(N1_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '2') {
    print_to_screen_char(N2_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '3') {
    print_to_screen_char(N3_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '4') {
    print_to_screen_char(N4_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '5') {
    print_to_screen_char(N5_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '6') {
    print_to_screen_char(N6_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '7') {
    print_to_screen_char(N7_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '8') {
    print_to_screen_char(N8_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '9') {
    print_to_screen_char(N9_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '$') {
    print_to_screen_char($_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == '%') {
    print_to_screen_char(percent_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == ',') {
    print_to_screen_char(comma_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '.') {
    print_to_screen_char(point_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == ':') {
    print_to_screen(two_points_font_0, width, height, x, y, font_size,
                    width * 0.1);
  } else if (c == '!') {
    print_to_screen_char(exclamation_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '?') {
    print_to_screen_char(question_mark_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '(') {
    print_to_screen_char(open_braket_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == ')') {
    print_to_screen_char(close_braket_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '-') {
    print_to_screen_char(minus_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '+') {
    print_to_screen_char(plus_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '*') {
    print_to_screen_char(times_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == '/') {
    print_to_screen_char(devide_font_0, width, height, x, y, font_size,
                         width * 0.1);
  } else if (c == 'A') {
    print_to_screen_char(A_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'B') {
    print_to_screen_char(B_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'C') {
    print_to_screen_char(C_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'D') {
    print_to_screen_char(D_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'E') {
    print_to_screen_char(E_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'F') {
    print_to_screen_char(F_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'G') {
    print_to_screen_char(G_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'H') {
    print_to_screen_char(H_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'I') {
    print_to_screen_char(I_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'J') {
    print_to_screen_char(J_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'K') {
    print_to_screen_char(K_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'L') {
    print_to_screen_char(L_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'M') {
    print_to_screen_char(M_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'N') {
    print_to_screen_char(N_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'O') {
    print_to_screen_char(O_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'P') {
    print_to_screen_char(P_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'Q') {
    print_to_screen_char(Q_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'R') {
    print_to_screen_char(R_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'S') {
    print_to_screen_char(S_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'T') {
    print_to_screen_char(T_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'U') {
    print_to_screen_char(U_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'V') {
    print_to_screen_char(V_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'W') {
    print_to_screen_char(W_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'X') {
    print_to_screen_char(X_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'Y') {
    print_to_screen_char(Y_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == 'Z') {
    print_to_screen_char(Z_font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == '_') {
    print_to_screen_char(__font_0, width, height, x, y, font_size, width * 0.1);
  } else if (c == '~') {
    print_to_screen_char(shift_font_0, width, height, x, y, font_size,
                         width * 0.1);
  }
}

/**
 * @brief Fills the entire ILI9488 screen with blue color.
 *
 * Sets the drawing region to full screen resolution and configures the pixel format.
 * Sends the color data to fill each pixel with blue.
 */

void FillScreenBlue() {

  set_resolution_pos(0, 0, 480, 320, 0);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x01);

  send_command(0x2C);

  for (uint64_t i = 0; i < 480 * 320; i++)
    send_ILI9488_data(0x09);
}

/**
 * @brief Fills the entire ILI9488 screen with black color.
 *
 * Sets the drawing region to full screen resolution and configures the pixel format.
 * Sends the color data to fill each pixel with black.
 */

void FillScreenblack() {

  set_resolution_pos(0, 0, 480, 320, 0);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x01);

  send_command(0x2C);

  for (uint64_t i = 0; i < 480 * 320; i++)
    send_ILI9488_data(0x00);
}

/**
 * @brief Clears previously drawn screen regions stored in history by filling them with black.
 *
 * Iterates backward through stored coordinates, sets the drawing area for each,
 * and overwrites the pixels with black color to effectively erase the content.
 * Resets the coordinate index after clearing.
 */

void clean_screen() {

  while (coord_index > 0) {

    send_command(0x3A); // interface pixel format
    send_ILI9488_data(0x06);

    uint16_t x_end = history[coord_index].x + history[coord_index].width -
                     1; // Dereference x to get the current value and modify it
    uint16_t y_end =
        history[coord_index].y +
        history[coord_index]
            .height; // Dereference y to get the current value and modify it

    // Split into high and low bytes
    uint8_t x_start_high = (history[coord_index].x >> 8) & 0xFF;
    uint8_t x_start_low = history[coord_index].x & 0xFF;
    uint8_t x_end_high = (x_end >> 8) & 0xFF;
    uint8_t x_end_low = x_end & 0xFF;

    uint8_t y_start_high = (history[coord_index].y >> 8) & 0xFF;
    uint8_t y_start_low = history[coord_index].y & 0xFF;
    uint8_t y_end_high = (y_end >> 8) & 0xFF;
    uint8_t y_end_low = y_end & 0xFF;

    send_command(0x2A); // Column Address Set
    send_ILI9488_data(x_start_high);
    send_ILI9488_data(x_start_low);
    send_ILI9488_data(x_end_high);
    send_ILI9488_data(x_end_low);

    send_command(0x2B); // Page Address Set
    send_ILI9488_data(y_start_high);
    send_ILI9488_data(y_start_low);
    send_ILI9488_data(y_end_high);
    send_ILI9488_data(y_end_low);

    send_command(0x3A); // interface pixel format
    send_ILI9488_data(0x01);

    send_command(0x2C);

    for (uint64_t i = 0;
         i < history[coord_index].width * history[coord_index].height; i++) {
      send_ILI9488_data(0x00);
    }
    coord_index--;
  }
  coord_index = 1;
}
/**
 * @brief Clears the screen area corresponding to a specific history element by filling it with black.
 *
 * Sets the drawing region based on the coordinates and dimensions stored in the given history index,
 * then fills that region with black pixels to erase the content.
 *
 * @param coord_index Index of the history element to clear; must be greater than 0.
 */

void clean_last_element_modified(uint8_t coord_index) {

  if (coord_index > 0) {
    send_command(0x3A); // interface pixel format
    send_ILI9488_data(0x06);

    uint16_t x_end = history[coord_index].x + history[coord_index].width -
                     1; // Dereference x to get the current value and modify it
    uint16_t y_end = history[coord_index].y + history[coord_index].height;

    uint8_t x_start_high = (history[coord_index].x >> 8) & 0xFF;
    uint8_t x_start_low = history[coord_index].x & 0xFF;
    uint8_t x_end_high = (x_end >> 8) & 0xFF;
    uint8_t x_end_low = x_end & 0xFF;

    uint8_t y_start_high = (history[coord_index].y >> 8) & 0xFF;
    uint8_t y_start_low = history[coord_index].y & 0xFF;
    uint8_t y_end_high = (y_end >> 8) & 0xFF;
    uint8_t y_end_low = y_end & 0xFF;

    send_command(0x2A);
    send_ILI9488_data(x_start_high);
    send_ILI9488_data(x_start_low);
    send_ILI9488_data(x_end_high);
    send_ILI9488_data(x_end_low);

    send_command(0x2B);
    send_ILI9488_data(y_start_high);
    send_ILI9488_data(y_start_low);
    send_ILI9488_data(y_end_high);
    send_ILI9488_data(y_end_low);

    send_command(0x3A);
    send_ILI9488_data(0x01);

    send_command(0x2C);

    for (uint64_t i = 0;
         i < history[coord_index].width * history[coord_index].height; i++) {
      send_ILI9488_data(0x00);
    }
  }
}

/**
 * @brief Clears the screen area of the last drawn element stored in history by filling it with black.
 *
 * Decrements the global coord_index, sets the drawing region based on the updated history entry,
 * and fills that region with black pixels to erase the last element from the display.
 * If coord_index is already 0 or less, it resets coord_index to 1.
 */

void clean_last_element() {

  if (coord_index > 0) {
    coord_index--;
    send_command(0x3A); // interface pixel format
    send_ILI9488_data(0x06);

    uint16_t x_end = history[coord_index].x + history[coord_index].width -
                     1; // Dereference x to get the current value and modify it
    uint16_t y_end =
        history[coord_index].y +
        history[coord_index]
            .height; // Dereference y to get the current value and modify it

    // Split into high and low bytes
    uint8_t x_start_high = (history[coord_index].x >> 8) & 0xFF;
    uint8_t x_start_low = history[coord_index].x & 0xFF;
    uint8_t x_end_high = (x_end >> 8) & 0xFF;
    uint8_t x_end_low = x_end & 0xFF;

    uint8_t y_start_high = (history[coord_index].y >> 8) & 0xFF;
    uint8_t y_start_low = history[coord_index].y & 0xFF;
    uint8_t y_end_high = (y_end >> 8) & 0xFF;
    uint8_t y_end_low = y_end & 0xFF;

    send_command(0x2A); // Column Address Set
    send_ILI9488_data(x_start_high);
    send_ILI9488_data(x_start_low);
    send_ILI9488_data(x_end_high);
    send_ILI9488_data(x_end_low);

    send_command(0x2B); // Page Address Set
    send_ILI9488_data(y_start_high);
    send_ILI9488_data(y_start_low);
    send_ILI9488_data(y_end_high);
    send_ILI9488_data(y_end_low);

    send_command(0x3A); // interface pixel format
    send_ILI9488_data(0x01);

    send_command(0x2C);

    for (uint64_t i = 0;
         i < history[coord_index].width * history[coord_index].height; i++) {
      send_ILI9488_data(0x00);
    }

  } else {
    coord_index = 1;
  }
}

/**
 * @brief Erases the last drawn character on the display by clearing its area to black.
 *
 * Decrements the global coord_index_char, sets the drawing region based on the updated
 * history_char entry, and fills that region with black pixels to remove the last character.
 * Does nothing if coord_index_char is already 0 or less.
 */

void clean_last_char() {

  if (coord_index_char > 0) {
    coord_index_char--;

    send_command(0x3A); // interface pixel format
    send_ILI9488_data(0x06);

    uint16_t x_end = history_char[coord_index_char].x +
                     history_char[coord_index_char].width -
                     1; // Dereference x to get the current value and modify it
    uint16_t y_end =
        history_char[coord_index_char].y +
        history_char[coord_index_char]
            .height; // Dereference y to get the current value and modify it

    // Split into high and low bytes
    uint8_t x_start_high = (history_char[coord_index_char].x >> 8) & 0xFF;
    uint8_t x_start_low = history_char[coord_index_char].x & 0xFF;
    uint8_t x_end_high = (x_end >> 8) & 0xFF;
    uint8_t x_end_low = x_end & 0xFF;

    uint8_t y_start_high = (history_char[coord_index_char].y >> 8) & 0xFF;
    uint8_t y_start_low = history_char[coord_index_char].y & 0xFF;
    uint8_t y_end_high = (y_end >> 8) & 0xFF;
    uint8_t y_end_low = y_end & 0xFF;

    send_command(0x2A); // Column Address Set
    send_ILI9488_data(x_start_high);
    send_ILI9488_data(x_start_low);
    send_ILI9488_data(x_end_high);
    send_ILI9488_data(x_end_low);

    send_command(0x2B); // Page Address Set
    send_ILI9488_data(y_start_high);
    send_ILI9488_data(y_start_low);
    send_ILI9488_data(y_end_high);
    send_ILI9488_data(y_end_low);

    send_command(0x3A); // interface pixel format
    send_ILI9488_data(0x01);

    send_command(0x2C);

    for (uint64_t i = 0; i < history_char[coord_index_char].width *
                                 history_char[coord_index_char].height;
         i++) {
      send_ILI9488_data(0x00);
    }
  }
}

/**
 * @brief Draws a virtual keyboard on the display with keys depending on mode (lowercase, uppercase, symbols).
 *
 * Initializes key positions, sizes, and labels for three rows plus special keys, 
 * clears their display areas, and renders the key labels. Also sets up special keys like Shift, Delete, OK, and Close.
 */

void draw_keyborad(char c) {

  gpio_set_level(SS_display, 0);

  background_color = "red";

  char *qwerty_row1;
  char *qwerty_row2;
  char *qwerty_row3;
  char *shift = "~";

  if (c == 'l') {
    qwerty_row1 = "qwertyuiop";
    qwerty_row2 = "asdfghjkl";
    qwerty_row3 = "zxcvbnm";
  } else if (c == 'u') {
    qwerty_row1 = "QWERTYUIOP";
    qwerty_row2 = "ASDFGHJKL";
    qwerty_row3 = "ZXCVBNM";
  } else if (c == 's') {
    qwerty_row1 = "1234567890";
    qwerty_row2 = ".,_?!$%()";
    qwerty_row3 = "-+*/  ";
    shift = " ";
  } else {
    return;
  }

  uint8_t j = 0;
  uint16_t x = 0;

  while (j < 10) {

    keyboard[j].x = x + 10;
    keyboard[j].y = 175;
    keyboard[j].width = 37;
    keyboard[j].height = 45;

    if (keyboard[j].label != NULL) {
      free(keyboard[j].label);
      keyboard[j].label = NULL;
    }

    keyboard[j].label = malloc(
        2 *
        sizeof(char)); // Allocate space for the character and null terminator
    keyboard[j].label[0] =
        qwerty_row1[j]; // Assign the character from qwerty_row1
    keyboard[j].label[1] = '\0';

    set_resolution_pos(x + 10, 175, 37, 45, 0);
    send_command(0x2C);

    for (uint64_t i = 0; i < 37 * 45 / 2; i++)
      send_ILI9488_data(0x24);

    print_ILI9488((char[]){qwerty_row1[j], '\0'}, x + 20, 180, 2);

    x = x + 47;
    j++;
  }

  x = 0;

  keyboard[j].x = x + 5;
  keyboard[j].y = 225;
  keyboard[j].width = 37;
  keyboard[j].height = 45;
  keyboard[j].label = "?12";

  j = 0;
  set_resolution_pos(x + 5, 225, 37, 45, 0);
  send_command(0x2C);

  for (uint64_t i = 0; i < 37 * 45 / 2; i++)
    send_ILI9488_data(0x24);

  print_ILI9488("?12", x + 8, 235, 1);

  while (j < 9) {

    // Save key position and dimensions
    keyboard[11 + j].x = x + 50;
    keyboard[11 + j].y = 225;
    keyboard[11 + j].width = 37;
    keyboard[11 + j].height = 45;

    if (keyboard[11 + j].label != NULL) {
      free(keyboard[11 + j].label);
      keyboard[11 + j].label = NULL;
    }

    keyboard[11 + j].label = malloc(
        2 *
        sizeof(char)); // Allocate space for the character and null terminator
    keyboard[11 + j].label[0] =
        qwerty_row2[j]; // Assign the character from qwerty_row1
    keyboard[11 + j].label[1] = '\0';

    set_resolution_pos(x + 50, 225, 37, 45, 0);
    send_command(0x2C);

    for (uint64_t i = 0; i < 37 * 45 / 2; i++)
      send_ILI9488_data(0x24);

    print_ILI9488((char[]){qwerty_row2[j], '\0'}, x + 60, 230, 2);

    x = x + 47;
    j++;
  }
  x = 0;

  // Save key position and dimensions
  keyboard[11 + j].x = x + 5;
  keyboard[11 + j].y = 274;
  keyboard[11 + j].width = 37;
  keyboard[11 + j].height = 45;
  keyboard[11 + j].label = shift;

  j = 0;

  set_resolution_pos(x + 5, 274, 37, 45, 0);
  send_command(0x2C);

  for (uint64_t i = 0; i < 37 * 45 / 2; i++)
    send_ILI9488_data(0x24);
  print_ILI9488(shift, x + 12, 279, 2);

  while (j < 7) {

    keyboard[21 + j].x = x + 50;
    keyboard[21 + j].y = 274;
    keyboard[21 + j].width = 37;
    keyboard[21 + j].height = 45;

    if (keyboard[21 + j].label != NULL) {
      free(keyboard[21 + j].label);
      keyboard[21 + j].label = NULL;
    }

    keyboard[21 + j].label = malloc(
        2 *
        sizeof(char)); // Allocate space for the character and null terminator
    keyboard[21 + j].label[0] =
        qwerty_row3[j]; // Assign the character from qwerty_row1
    keyboard[21 + j].label[1] = '\0';

    set_resolution_pos(x + 50, 274, 37, 45, 0);
    send_command(0x2C);

    for (uint64_t i = 0; i < 37 * 45 / 2; i++)
      send_ILI9488_data(0x24);
    print_ILI9488((char[]){qwerty_row3[j], '\0'}, x + 60, 279, 2);

    x = x + 47;
    j++;
  }

  keyboard[21 + j].x = x + 47;
  keyboard[21 + j].y = 274;
  keyboard[21 + j].width = 60;
  keyboard[21 + j].height = 43;
  keyboard[21 + j].label = " ";

  set_resolution_pos(x + 47, 274, 60, 43, 0);
  send_command(0x2C);

  for (uint64_t i = 0; i < 60 * 43 / 2; i++)
    send_ILI9488_data(0x24);
  j++;
  x = x + 47;

  keyboard[21 + j].x = x + 65;
  keyboard[21 + j].y = 274;
  keyboard[21 + j].width = 37;
  keyboard[21 + j].height = 43;
  keyboard[21 + j].label = "DEL";

  set_resolution_pos(x + 65, 274, 37, 43, 0);
  send_command(0x2C);

  for (uint64_t i = 0; i < 37 * 43 / 2; i++)
    send_ILI9488_data(0x24);

  print_ILI9488("DEL", x + 67, 285, 1);

  j++;

  keyboard[21 + j].x = 456;
  keyboard[21 + j].y = 0;
  keyboard[21 + j].width = 24;
  keyboard[21 + j].height = 29;
  keyboard[21 + j].label = "close";

  background_color = "red";
  print_ILI9488("X", 456, 0, 2);

  j++;

  keyboard[21 + j].x = 0;
  keyboard[21 + j].y = 0;
  keyboard[21 + j].width = 30;
  keyboard[21 + j].height = 40;
  keyboard[21 + j].label = "OK";

  background_color = "green";
  print_ILI9488("O", 0, 0, 2);
  send_command(0x00);
  background_color = "red";

  gpio_set_level(SS_display, 1);
}

/**
 * @brief Draws main menu icons on the display and updates the current history entry's app name accordingly.
 *
 * For each app icon (notebook, pong, goblin_royale, GPIO_C), sets drawing area, pixel format,
 * sends the icon pixel data to the display, and stores the app name in the history at coord_index.
 */

void draw_main_menu_icons() {

  gpio_set_level(SS_display, 0);

  strncpy(history[coord_index].app_name, "notebook", APP_NAME_MAX_LEN);

  set_resolution_pos(10, 10, 67, 76, 0);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x06);

  send_command(0x2C);

  for (uint64_t i = 0; i < size_var1; i++)
    send_ILI9488_data(notebook[i]);

  send_command(0x00);

  strncpy(history[coord_index].app_name, "pong", APP_NAME_MAX_LEN);

  set_resolution_pos(97, 10, 67, 76, 0);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x06);

  send_command(0x2C);

  for (uint64_t i = 0; i < size_var2; i++)
    send_ILI9488_data(pong_logo[i]);

  send_command(0x00);

  strncpy(history[coord_index].app_name, "goblin_royale", APP_NAME_MAX_LEN);

  set_resolution_pos(184, 10, 67, 76, 0);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x06);

  send_command(0x2C);

  for (uint64_t i = 0; i < size_var2; i++)
    send_ILI9488_data(goblin_royale_logo[i]);

  send_command(0x00);

  strncpy(history[coord_index].app_name, "GPIO_C", APP_NAME_MAX_LEN);

  set_resolution_pos(271, 10, 67, 76, 0);

  send_command(0x3A); // interface pixel format
  send_ILI9488_data(0x06);

  send_command(0x2C);

  for (uint64_t i = 0; i < size_var2; i++)
    send_ILI9488_data(gpio_C_logo[i]);

  send_command(0x00);

  gpio_set_level(SS_display, 1);
}
/**
 * @brief Draws a red "X" button on the display and records its position and size in history_char.
 *
 * Sets the background color, updates history_char with the button's metadata, prints the "X",
 * and manages the display chip select during drawing.
 */

void make_X_button() {
  background_color = "red";

  gpio_set_level(SS_display, 0);

  strcpy(history_char[coord_index_char].app_name, "close");
  history_char[coord_index_char].x = 456;
  history_char[coord_index_char].y = 0;
  history_char[coord_index_char].width = 24;
  history_char[coord_index_char].height = 29;

  coord_index_char++;

  background_color = "red";
  print_ILI9488("X", 456, 0, 2);
  background_color = "red";

  send_command(0x00);
  gpio_set_level(SS_display, 1);
}

/**
 * @brief Draws a colored rectangular button with a given label and updates its metadata in history_char.
 *
 * Determines color code from the color string, sets the drawing area based on button size,
 * fills the area with the color, prints the button text, adjusts coord_index based on spaces in the label,
 * and manages display chip select signals during the process.
 */

void make_button(char *name, uint16_t height, uint16_t x, uint16_t y,
                 char *color) {
  int len = strlen(name);
  uint8_t color_hex;

  if (strcmp(color, "red") == 0)
    color_hex = 0x24;

  else
    color_hex = 0x12;

  gpio_set_level(SS_display, 0);

  strcpy(history_char[coord_index_char].app_name, name);
  history_char[coord_index_char].x = x;
  history_char[coord_index_char].y = y;
  history_char[coord_index_char].width = 24 + 24 * len - 1;
  history_char[coord_index_char].height = height;

  set_resolution_pos(x, y, (24 + (24 * len - 1)), height, 0);

  send_command(0x2C);

  for (uint64_t i = 0; i < (24 + (24 * len - 1)) * height / 2; i++)
    send_ILI9488_data(color_hex);

  background_color = color;

  print_ILI9488(name, x + 15, y + 15, 2);

  int space_count = 0;

  for (int i = 0; i < len; i++)
    if (name[i] == ' ')
      space_count++;

  coord_index = coord_index - len + space_count;

  coord_index_char++;

  send_command(0x00);

  gpio_set_level(SS_display, 1);
}

/**
 * @brief Initializes the notebook app interface by drawing a close button and three action buttons.
 *
 * Sets up the "X" close button metadata and draws it, then creates "New file", "Read files", and "Edit files"
 * buttons at specified positions with red backgrounds, managing display chip select and command signals accordingly.
 */

void bootApp_noteBook() {

  uint16_t width = 24;
  uint16_t height = 29;
  uint16_t x = 456;
  uint16_t y = 0;

  gpio_set_level(SS_display, 0);

  strcpy(history_char[coord_index_char].app_name, "close");
  history_char[coord_index_char].x = x;
  history_char[coord_index_char].y = y;
  history_char[coord_index_char].width = width;
  history_char[coord_index_char].height = height;

  coord_index_char++;

  background_color = "red";
  print_ILI9488("X", 456, 0, 2);

  background_color = "red";

  height = 70;
  x = 135;

  make_button("New file", height, x, 30, "red");

  make_button("Read files", height, x, 120, "red");

  make_button("Edit files", height, x, 210, "red");

  send_command(0x00);

  gpio_set_level(SS_display, 1);
}

/**
 * @brief Initializes the GPIO_C app interface by drawing a close button and three communication protocol buttons.
 *
 * Sets the metadata and draws the "X" close button, then creates "SPI", "I2C", and "UART" buttons with red backgrounds
 * at specified coordinates, controlling the display chip select and command signals accordingly.
 */

void GPIO_C_boot() {

  uint16_t width = 24;
  uint16_t height = 29;
  uint16_t x = 456;
  uint16_t y = 0;

  gpio_set_level(SS_display, 0);

  strcpy(history_char[coord_index_char].app_name, "close");
  history_char[coord_index_char].x = x;
  history_char[coord_index_char].y = y;
  history_char[coord_index_char].width = width;
  history_char[coord_index_char].height = height;

  coord_index_char++;

  background_color = "red";
  print_ILI9488("X", 456, 0, 2);

  height = 70;
  x = 200;

  make_button("SPI", height, x, 30, "red");

  make_button("I2C", height, x, 120, "red");

  make_button("UART", height, x, 210, "red");

  send_command(0x00);

  gpio_set_level(SS_display, 1);
}

/**
 * @brief Initializes and displays GPIO control buttons and a close button.
 * @note Uses global display state (history_char, coord_index_char, background_color).
 */

void GPIO_pins_boot() {

  uint16_t width = 24;
  uint16_t height = 29;
  uint16_t x = 456;
  uint16_t y = 0;

  gpio_set_level(SS_display, 0);

  strcpy(history_char[coord_index_char].app_name, "close");
  history_char[coord_index_char].x = x;
  history_char[coord_index_char].y = y;
  history_char[coord_index_char].width = width;
  history_char[coord_index_char].height = height;

  coord_index_char++;

  background_color = "red";
  print_ILI9488("X", 456, 0, 2);

  height = 55;

  make_button("GPIO 0", height, 50, 60, "red");

  make_button("GPIO 2", height, 240, 60, "red");

  make_button("GPIO 12", height, 120, 150, "red");

  send_command(0x00);

  gpio_set_level(SS_display, 1);
}

/**
 * @brief Displays a confirmation screen with a close button and transmit button.
 * @note Manages display synchronization (SS_display) and global UI state.
 */

void confirm_boot() {

  uint16_t width = 24;
  uint16_t height = 29;
  uint16_t x = 456;
  uint16_t y = 0;

  gpio_set_level(SS_display, 0);

  strcpy(history_char[coord_index_char].app_name, "close");
  history_char[coord_index_char].x = x;
  history_char[coord_index_char].y = y;
  history_char[coord_index_char].width = width;
  history_char[coord_index_char].height = height;

  coord_index_char++;

  background_color = "red";
  print_ILI9488("X", 456, 0, 2);

  height = 55;

  make_button("Transmit", height, 120, 110, "red");

  send_command(0x00);

  gpio_set_level(SS_display, 1);
}

/**
 * @brief Initializes I2C confirmation screen with close, write and read buttons.
 * @note Controls display hardware (SS_display) and updates UI state globals.
 */
void confirm_boot_I2C() {

  uint16_t width = 24;
  uint16_t height = 29;
  uint16_t x = 456;
  uint16_t y = 0;

  gpio_set_level(SS_display, 0);

  strcpy(history_char[coord_index_char].app_name, "close");
  history_char[coord_index_char].x = x;
  history_char[coord_index_char].y = y;
  history_char[coord_index_char].width = width;
  history_char[coord_index_char].height = height;

  coord_index_char++;

  background_color = "red";
  print_ILI9488("X", 456, 0, 2);

  height = 55;

  make_button("Write", height, 50, 110, "red");
  make_button("Read", height, 240, 110, "red");

  send_command(0x00);

  gpio_set_level(SS_display, 1);
}

/**
 * @brief Prints a formatted table of history records.
 * @details Displays all non-empty history entries in a table format with columns:
 *          ID, X, Y, Width, Height, Correction, and App Name.
 */
void print_history_table() {
  printf("---------------------------------------------------------------------"
         "-------------------------------\n");
  printf("| %-3s | %-5s | %-5s | %-6s | %-7s | %-10s | %-32s |\n", "ID", "X",
         "Y", "Width", "Height", "Correction", "App Name");
  printf("---------------------------------------------------------------------"
         "-------------------------------\n");

  for (int i = 0; i < 50; i++) {
    if (history[i].app_name[0] != '\0') {
      printf("| %-3d | %-5u | %-5u | %-6u | %-7u | %-10d | %-32s |\n", i,
             history[i].x, history[i].y, history[i].width, history[i].height,
             history[i].correction, history[i].app_name);
    }
  }

  printf("---------------------------------------------------------------------"
         "-------------------------------\n");
}

/**
 * @brief Prints a formatted table of character history records.
 * @details Displays all non-empty history_char entries in a table format with columns:
 *          ID, X, Y, Width, Height, Correction, and App Name.
 */

void print_history_char_table() {
  printf("---------------------------------------------------------------------"
         "-------------------------------\n");
  printf("| %-3s | %-5s | %-5s | %-6s | %-7s | %-10s | %-32s |\n", "ID", "X",
         "Y", "Width", "Height", "Correction", "App Name");
  printf("---------------------------------------------------------------------"
         "-------------------------------\n");

  for (int i = 0; i < 50; i++) {
    if (history_char[i].app_name[0] != '\0') {
      printf("| %-3d | %-5u | %-5u | %-6u | %-7u | %-10d | %-32s |\n", i,
             history_char[i].x, history_char[i].y, history_char[i].width,
             history_char[i].height, history_char[i].correction,
             history_char[i].app_name);
    }
  }

  printf("---------------------------------------------------------------------"
         "-------------------------------\n");
}
