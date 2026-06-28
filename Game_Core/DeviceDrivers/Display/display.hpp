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

void print_clear(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h);

void print_moved_clr(uint16_t x_center, uint16_t y_center, uint16_t w, uint16_t h, bool moved, uint16_t dir,  uint16_t speed);

void print_moved_img(uint16_t x_center, uint16_t y_center, uint16_t w, uint16_t h, bool moved, uint16_t dir, uint16_t speed, uint16_t data[]);

void print_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h,  uint16_t data[]);