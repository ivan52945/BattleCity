#pragma once

#include <cstdint>

//TODO: add display choosing
#include "st7789.h"

#define DISPLAY_W ST7789_WIDTH
#define DISPLAY_H ST7789_HEIGHT

enum side_t {
    UP, RIGHT, DOWN, LEFT,
  };

void display_init();

void display_print_img(uint16_t x, uint16_t y, uint16_t w, uint16_t h,  uint16_t data[]);