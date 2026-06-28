#include <cstdint>
#include "display.hpp"

#include <cstring>

#include "st7789.h"

__attribute__((aligned(32))) uint16_t img_clear[32];

void display_init()
{
    ST7789_Init();
    memset(img_clear, 0, sizeof(img_clear));
    ST7789_Fill_Color(BLACK);
}

void print_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t data[])
{
    ST7789_DrawImage(x, y, w, h, data);
}

void print_clear(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    ST7789_DrawImage(x, y, w, h, img_clear);
}

void print_moved_img(uint16_t x_center, uint16_t y_center, uint16_t w, uint16_t h, bool moved, uint16_t dir, uint16_t speed, uint16_t data[])
{
    const uint16_t w_r = w / 2;
    const uint16_t h_r = h / 2;

    if (moved) {
        switch (dir) {
            case UP:
                print_clear(x_center - w_r, y_center + (h_r + 1), w, speed);
                break;
            case DOWN:
                print_clear(x_center - w_r, y_center - (h_r + speed), w, speed);
                break;
            case LEFT:
                print_clear(x_center + (w_r + 1), y_center - h_r, speed, h);
                break;
            case RIGHT:
                print_clear(x_center - (w_r + speed), y_center - h_r, speed, h);
                break;
            default: break;
        }
    }

    print_image(x_center - w_r, y_center - h_r, w, h, data);
}

void print_moved_clr(uint16_t x_center, uint16_t y_center, uint16_t w, uint16_t h, bool moved, uint16_t dir, uint16_t speed)
{
    const uint16_t w_r = w / 2;
    const uint16_t h_r = h / 2;

    switch (dir) {
        case UP:
            y_center += speed;
            break;
        case DOWN:
            y_center -= speed;
            break;
        case LEFT:
            x_center += speed;
            break;
        case RIGHT:
            x_center -= speed;
            break;
        default: break;
    }
    print_image(x_center - w_r, y_center - h_r, w, h, img_clear);
}
