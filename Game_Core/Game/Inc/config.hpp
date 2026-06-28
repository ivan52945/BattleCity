#pragma once

#include "point.hpp"
#include "display.hpp"

#define FIELD_WIDTH DISPLAY_W
#define FIELD_HEIGHT DISPLAY_H

constexpr point_t tank_spawns[2][7] = {
  {
    {.x = 30, .y = 30 },
    {.x = 30, .y = 60 },
    {.x = 30, .y = 90 },
    {.x = 30, .y = 120 },
//    {.x = 30, .y = 150 },
//    {.x = 30, .y = 180 },
//    {.x = 30, .y = 210 },
  },
  {
    {.x = 210, .y = 30 },
    {.x = 210, .y = 60 },
    {.x = 210, .y = 90 },
    {.x = 210, .y = 120 },
//    {.x = 210, .y = 150 },
//    {.x = 210, .y = 180 },
//    {.x = 210, .y = 210 },
  },
};