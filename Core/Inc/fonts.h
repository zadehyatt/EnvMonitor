#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

// Font structure
typedef struct {
    const uint8_t FontWidth;    // Font width in pixels
    const uint8_t FontHeight;   // Font height in pixels
    const uint8_t *data;        // Pointer to font data array (changed from uint16_t to uint8_t)
} FontDef;

// Font 5x8 pixels (standard)
extern FontDef Font_5x8;

// Font 7x10 pixels (currently uses 5x8 data)
extern FontDef Font_7x10;

// Font 6x8 pixels (currently uses 5x8 data)
extern FontDef Font_6x8;

#endif // FONTS_H
