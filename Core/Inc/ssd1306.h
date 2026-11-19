#ifndef SSD1306_H
#define SSD1306_H

#include "stm32f4xx_hal.h"
#include "fonts.h"

// I2C address (default is 0x3C, some modules use 0x3D)
#define SSD1306_I2C_ADDR        0x78  // 0x3C << 1
#define SSD1306_I2C_ADDR_ALT    0x7A  // 0x3D << 1

// SSD1306 width in pixels
#define SSD1306_WIDTH           64
// SSD1306 height in pixels
#define SSD1306_HEIGHT          32

// Enumeration for screen colors
typedef enum {
    Black = 0x00,   // Black color, no pixel
    White = 0x01    // Pixel is set. Color depends on OLED
} SSD1306_COLOR;

// Struct to store transformations
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
} SSD1306_t;

// Initialize the oled screen
uint8_t ssd1306_Init(I2C_HandleTypeDef *hi2c);

// Fill the whole screen with the given color
void ssd1306_Fill(SSD1306_COLOR color);

// Write the screenbuffer with changed to the screen
void ssd1306_UpdateScreen(I2C_HandleTypeDef *hi2c);

// Draw one pixel in the screenbuffer
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);

// Draw a string at the current position
char ssd1306_WriteString(char* str, FontDef Font, SSD1306_COLOR color);

// Position the cursor
void ssd1306_SetCursor(uint8_t x, uint8_t y);

// Clear screen
void ssd1306_Clear(void);

// Test function - displays a pattern
void ssd1306_TestBorder(void);
void ssd1306_TestAll(void);

#endif // SSD1306_H
