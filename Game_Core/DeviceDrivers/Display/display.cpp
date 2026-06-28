#include <cstdint>
#include "display.hpp"
#include <cstring>

#include "st7789.h"

void display_init()
{
    ST7789_Init();
    ST7789_Fill_Color(BLACK);
}

void display_print_img(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t data[])
{
    ST7789_DrawImage(x, y, w, h, data);
}
