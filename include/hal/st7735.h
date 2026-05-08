#ifndef ST7735_H
#define ST7735_H

#include "hal/libraries.h"
#include "hal/spi.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160
#define RESET_PIN PB0
#define DATA_SELECT_PIN PD7

// Makra pro rychlé přepínání stavu řídících pinů
#define ST7735_CS_LOW()  PORTB &= ~(1 << SPI_CS)
#define ST7735_CS_HIGH() PORTB |=  (1 << SPI_CS)

#define ST7735_DC_CMD()  PORTD &= ~(1 << DATA_SELECT_PIN) // Command mode
#define ST7735_DC_DATA() PORTD |=  (1 << DATA_SELECT_PIN) // Data mode

#define ST7735_RST_LOW()  PORTB &= ~(1 << RESET_PIN)
#define ST7735_RST_HIGH() PORTB |=  (1 << RESET_PIN)

// Define colors
#define COLOR_GREEN		0x07E0
#define COLOR_BLUE		0x001F
#define COLOR_RED		0xF800
#define COLOR_WHITE		0xFFFF
#define COLOR_CYAN		0x07FF
#define COLOR_ORANGE	0xFD20
#define COLOR_MAGENTA   0xF81F
#define COLOR_BG        0x0000


// 5x7 font (ASCII characters from 32 ' ' to 126 '~')
#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
#define ADD_BLANK_LINE 1 


/**
 * @brief Performs hardware reset and basic initialization sequence for the ST7735 controller.
 * * This function handles the low-level startup of the display:
 * - **Bus Initialization:** Starts the SPI communication bus.
 * - **GPIO Configuration:** Sets the Reset (RST) and Data/Command (DC) pins as outputs.
 * - **Hardware Reset:** Executes a precise timed toggle of the RST pin to clear the 
 * controller's internal registers.
 * - **Command Sequence:** * - Sends *Software Reset* (0x01) and *Sleep Out* (0x11).
 * - Configures *Color Mode* (0x3A) to 16-bit RGB565 format.
 * - Sets *Memory Access Control* (0x36) to rotate the screen 180 degrees.
 * - **Finalization:** Fills the screen with the background color and activates 
 * the display via the *Display ON* (0x29) command.
 * * @note This function includes several blocking delays (up to 150ms) as required 
 * by the ST7735 datasheet for stable operation.
 */
void st7735_init(void);


/**
 * @brief Sends a command byte to the ST7735 controller.
 * * Sets the Data/Command (DC) pin to logic LOW to signal that the incoming 
 * byte should be interpreted as an instruction. It manages the Chip Select (CS) 
 * line to ensure exclusive communication during the SPI transfer.
 * * @param cmd The 8-bit command instruction (opcode) to be sent.
 */
void st7735_write_command(uint8_t cmd);


/**
 * @brief Sends a data byte to the ST7735 controller.
 * * Sets the Data/Command (DC) pin to logic HIGH to signal that the incoming 
 * byte is a parameter or pixel data for a previously sent command. 
 * Manages the Chip Select (CS) line for the duration of the SPI transfer.
 * * @param data The 8-bit data byte to be sent.
 */
void st7735_write_data(uint8_t data);


/**
 * @brief Defines a rectangular window in the display's RAM for subsequent write operations.
 * * This function optimizes rendering by setting the "Address Window". Once defined, 
 * any following pixel data sent to the controller will automatically fill this 
 * area from top-left to bottom-right, eliminating the need to send coordinates 
 * for every individual pixel.
 * - **Boundary Check:** Validates that the requested area fits within the 
 * physical limits of the display (128x160).
 * - **CASET (0x2A):** Sets the column (X) start and end addresses.
 * - **RASET (0x2B):** Sets the row (Y) start and end addresses.
 * - **RAMWR (0x2C):** Sends the "Memory Write" instruction to prepare the 
 * controller for incoming color data.
 * * @param x,y Top-left starting coordinates.
 * @param w,h Width and height of the drawing window.
 * @return true if the area is within screen bounds; false otherwise.
 */
bool st7735_set_draw_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h);


/**
 * @brief Clears the entire display with a single uniform color.
 * * Sets the drawing area to the full resolution of the display (128x160) 
 * and streams the color data to every pixel. This is used for screen 
 * clears during initialization or state transitions.
 * * @param color The 16-bit RGB565 color value to fill the screen with.
 */
void st7735_fill_screen(uint16_t color);


/**
 * @brief Draws a solid filled rectangle at a specified location.
 * * Optimized for speed, this function sets a local drawing window and 
 * pushes pixel data directly into the display's RAM.
 * - **Boundary Safety:** Aborts if the rectangle dimensions are out of bounds.
 * - **Streaming:** Keeps the Chip Select (CS) low for the duration of the 
 * fill to maximize SPI throughput.
 * * @param x,y Top-left coordinates of the rectangle.
 * @param w,h Width and height in pixels.
 * @param color The 16-bit RGB565 color value for the fill.
 */
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);


/**
 * @brief Renders a single pixel at the specified coordinates.
 * * Sets a minimal 1x1 drawing area and pushes a single color value.
 * While less efficient for large shapes, it is essential for drawing 
 * lines, dots, or fine UI details.
 * * @param x,y The coordinate of the pixel.
 * @param color The 16-bit RGB565 color value.
 */
void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color);


/**
 * @brief Renders a pre-buffered 16-bit color sprite to the display.
 * * This function streams an array of color data from RAM into a 
 * defined drawing area. It is the primary method for rendering 
 * multi-colored objects or dynamic UI elements where every pixel 
 * color is already calculated in a buffer.
 * - **Efficiency:** Uses the address window to push a continuous 
 * stream of pixels without coordinate overhead.
 * * @param x,y Top-left starting coordinates.
 * @param w,h Dimensions of the object.
 * @param buffer Pointer to an array of 16-bit color values (RGB565).
 */
void st7735_draw_object(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t* buffer);


/**
 * @brief Renders an 8x8 monochrome bitmap stored in Flash memory (PROGMEM).
 * * Efficiently draws small sprites or icons by expanding 1-bit-per-pixel data 
 * into 16-bit color output. 
 * - **Memory Efficient:** Uses `memcpy_P` to fetch only 8 bytes of data into RAM.
 * - **Bit Parsing:** Iterates through each bit of the byte (row) and sends 
 * either the foreground or background color to the display.
 * * @param x,y Coordinates for the top-left corner.
 * @param bitmap_array Pointer to the 8-byte array in PROGMEM.
 * @param color 16-bit color for the set bits ('1').
 * @param bg_color 16-bit color for the unset bits ('0').
 */
void draw_8bit_PROGMEM(uint8_t x, uint8_t y, const void *bitmap_array, uint16_t color, uint16_t bg_color);


/**
 * @brief Renders a 16x16 monochrome bitmap stored in Flash memory (PROGMEM).
 * * Handles larger sprites (like the player's ship) by processing 16-bit rows.
 * - **Implementation:** Fetches 32 bytes from Flash and parses each 16-bit word 
 * bit-by-bit to determine the pixel color.
 * - **Optimization:** Uses the ST7735 address window to stream all 256 pixels 
 * in a single SPI transaction.
 * * @param x,y Coordinates for the top-left corner.
 * @param bitmap_array Pointer to the 32-byte (16x16 bit) array in PROGMEM.
 * @param color 16-bit color for the set bits ('1').
 * @param bg_color 16-bit color for the unset bits ('0').
 */
void draw_16bit_PROGMEM(uint8_t x, uint8_t y, const void *bitmap_array, uint16_t color, uint16_t bg_color);


/**
 * @brief Renders a text string using a high-performance row-major streaming technique.
 * * Instead of drawing characters individually, this function:
 * - **Optimizes SPI:** Sets a single drawing area for the entire string.
 * - **Batch Fetch:** Copies all required character bitmaps from PROGMEM to a 
 * local RAM buffer at once.
 * - **Row-Major Rendering:** Iterates through the vertical scanlines (rows) 
 * and streams pixel data for all characters in the string before moving 
 * to the next row. This keeps the SPI pipeline full.
 * - **Formatting:** Automatically adds a 1-pixel spacing between characters 
 * and an optional blank line at the bottom.
 * * @param x,y Top-left starting coordinates for the string.
 * @param buffer The null-terminated string to display.
 * @param len Manual length override (if 0, strlen is used).
 * @param color 16-bit foreground color.
 * @param bg_color 16-bit background color.
 */
void draw_char_buffer(int x, int y, const char* buffer, uint8_t len, uint16_t color, uint16_t bg_color);

#ifdef __cplusplus
    }
#endif

#endif // ST7735_H