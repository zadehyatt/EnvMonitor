#include "ssd1306.h"

// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Screen object
static SSD1306_t SSD1306;

// I2C handle
static I2C_HandleTypeDef *ssd1306_i2c;

// Commands
#define SSD1306_COMMAND         0x00
#define SSD1306_DATA            0x40

// Send a byte to the command register
static void ssd1306_WriteCommand(uint8_t byte) {
    HAL_I2C_Mem_Write(ssd1306_i2c, SSD1306_I2C_ADDR, SSD1306_COMMAND, 1, &byte, 1, HAL_MAX_DELAY);
}

// Initialize the oled screen
uint8_t ssd1306_Init(I2C_HandleTypeDef *hi2c) {
    ssd1306_i2c = hi2c;
    
    // Wait for the screen to boot
    HAL_Delay(100);
    
    // Check if OLED is connected
    if(HAL_I2C_IsDeviceReady(ssd1306_i2c, SSD1306_I2C_ADDR, 1, 20000) != HAL_OK) {
        // Try alternate address
        if(HAL_I2C_IsDeviceReady(ssd1306_i2c, SSD1306_I2C_ADDR_ALT, 1, 20000) != HAL_OK) {
            return 0;  // OLED not found
        }
    }
    
    // Init OLED - 64x32 specific (trying alternate COM configuration)
    ssd1306_WriteCommand(0xAE); // Display off
    ssd1306_WriteCommand(0xD5); // Set display clock divide ratio/oscillator frequency
    ssd1306_WriteCommand(0x80); // Set divide ratio
    ssd1306_WriteCommand(0xA8); // Set multiplex ratio
    ssd1306_WriteCommand(0x1F); // 1/32 duty (for 64x32 display)
    ssd1306_WriteCommand(0xD3); // Set display offset
    ssd1306_WriteCommand(0x00); // No offset
    ssd1306_WriteCommand(0x40); // Set start line address
    ssd1306_WriteCommand(0x8D); // Charge pump
    ssd1306_WriteCommand(0x14); // Enable charge pump
    ssd1306_WriteCommand(0x20); // Set Memory Addressing Mode
    ssd1306_WriteCommand(0x00); // Horizontal addressing mode
    ssd1306_WriteCommand(0xA0); // Set segment re-map 0 to 127 (normal, not remapped)
    ssd1306_WriteCommand(0xC0); // Set COM output scan direction (normal)
    ssd1306_WriteCommand(0xDA); // Set com pins hardware configuration
    ssd1306_WriteCommand(0x12); // Alternate COM pin config (trying this instead of 0x02)
    ssd1306_WriteCommand(0x81); // Set contrast control
    ssd1306_WriteCommand(0x8F); // Contrast value
    ssd1306_WriteCommand(0xD9); // Set pre-charge period
    ssd1306_WriteCommand(0xF1); // Pre-charge period
    ssd1306_WriteCommand(0xDB); // Set vcomh deselect level
    ssd1306_WriteCommand(0x40); // Vcomh deselect level
    ssd1306_WriteCommand(0xA4); // Output follows RAM content
    ssd1306_WriteCommand(0xA6); // Set normal display (not inverted)
    ssd1306_WriteCommand(0xAF); // Display on
    
    // Clear screen
    ssd1306_Fill(Black);
    
    // Flush buffer to screen
    ssd1306_UpdateScreen(hi2c);
    
    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;
    SSD1306.Initialized = 1;
    
    return 1;
}

// Fill the whole screen with the given color
void ssd1306_Fill(SSD1306_COLOR color) {
    uint32_t i;
    for(i = 0; i < sizeof(SSD1306_Buffer); i++) {
        SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

// Write the screenbuffer with changed to the screen
void ssd1306_UpdateScreen(I2C_HandleTypeDef *hi2c) {
    // Set addressing bounds for 64x32 display (with column offset for 128-wide controller)
    ssd1306_WriteCommand(0x21); // Set column address
    ssd1306_WriteCommand(0x20); // Column start address (32 - centered on 128-wide controller)
    ssd1306_WriteCommand(0x5F); // Column end address (32+63 = 95)
    
    ssd1306_WriteCommand(0x22); // Set page address
    ssd1306_WriteCommand(0x00); // Page start address (0)
    ssd1306_WriteCommand(0x03); // Page end address (3 for 32 pixels)
    
    // Send buffer in one go (horizontal addressing auto-increments)
    HAL_I2C_Mem_Write(hi2c, SSD1306_I2C_ADDR, SSD1306_DATA, 1, SSD1306_Buffer, sizeof(SSD1306_Buffer), HAL_MAX_DELAY);
}

// Draw one pixel in the screenbuffer
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }
    
    if(color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

// Draw a character
static char ssd1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color) {
    uint8_t i, j;
    uint8_t line;
    
    // Check available space
    if(SSD1306_WIDTH < (SSD1306.CurrentX + Font.FontWidth) ||
       SSD1306_HEIGHT < (SSD1306.CurrentY + Font.FontHeight)) {
        return 0;
    }
    
    // Draw character (5x8 font: 5 columns, each byte represents 8 vertical pixels)
    for(i = 0; i < Font.FontWidth; i++) {
        line = Font.data[(ch - 32) * Font.FontWidth + i];
        for(j = 0; j < Font.FontHeight; j++) {
            if(line & (1 << j)) {
                ssd1306_DrawPixel(SSD1306.CurrentX + i, SSD1306.CurrentY + j, (SSD1306_COLOR) color);
            } else {
                ssd1306_DrawPixel(SSD1306.CurrentX + i, SSD1306.CurrentY + j, (SSD1306_COLOR)!color);
            }
        }
    }
    
    // Update the current X position
    SSD1306.CurrentX += Font.FontWidth + 1;  // Add 1 pixel space between characters
    
    return ch;
}

// Write a string
char ssd1306_WriteString(char* str, FontDef Font, SSD1306_COLOR color) {
    while(*str) {
        if(ssd1306_WriteChar(*str, Font, color) != *str) {
            return *str;
        }
        str++;
    }
    return *str;
}

// Position the cursor
void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

// Clear screen (fill with black)
void ssd1306_Clear(void) {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
}

// Draw border for testing
void ssd1306_TestBorder(void) {
    ssd1306_Fill(Black);
    
    uint8_t x, y;
    for(x = 0; x < SSD1306_WIDTH; x++) {
        ssd1306_DrawPixel(x, 0, White);
        ssd1306_DrawPixel(x, SSD1306_HEIGHT - 1, White);
    }
    
    for(y = 0; y < SSD1306_HEIGHT; y++) {
        ssd1306_DrawPixel(0, y, White);
        ssd1306_DrawPixel(SSD1306_WIDTH - 1, y, White);
    }
}

// Test function - displays text
void ssd1306_TestAll(void) {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("OLED", Font_7x10, White);
    ssd1306_SetCursor(0, 12);
    ssd1306_WriteString("Works!", Font_7x10, White);
}
