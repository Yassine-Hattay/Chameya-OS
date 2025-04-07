#include "ILI9488_driver.h"

// Function to draw a character 'A' on the screen
void draw_char(char character) {
    // Define a 5x5 bitmap for the character 'A'
    uint8_t A_bitmap[5] = {
        0x1F,  // 11111
        0x11,  // 10001
        0x1F,  // 11111
        0x11,  // 10001
        0x11   // 10001
    };

    set_orientation(6);
    // Set the resolution and position where you want to draw the character
    set_resolution_pos(200, 110, 40, 40);  // Adjust coordinates as needed

    // Loop through each row of the bitmap (5 rows)
    for (int row = 0; row < 5; row++) {
        uint8_t row_data = A_bitmap[row];

        // Loop through each column (5 columns)
        for (int col = 0; col < 5; col++) {
            // Check if the pixel is on (1) or off (0)
            uint8_t pixel = (row_data >> (4 - col)) & 0x01;

            // Scale the pixel by creating a 5x5 block for each pixel
            for (int x = 0; x < 5; x++) {
                for (int y = 0; y < 5; y++) {
                    // Create the 8-bit RGBRGB value
                    uint8_t red = pixel ? 0x03 : 0x00;   // 0x03 for white, 0x00 for black
                    uint8_t green = pixel ? 0x03 : 0x00;
                    uint8_t blue = pixel ? 0x03 : 0x00;  // 0x03 for white, 0x00 for black

                    // Pack the RGB values into an 8-bit color format (RGBRGB)
                    uint8_t color = (red << 6) | (green << 4) | (blue << 2);

                    // Send the packed color value to the display
                    send_ILI9488_data(color);
                }
            }
        }
    }
}
