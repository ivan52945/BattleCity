#pragma once

#include <stdint.h>

#define SIDES           4
#define TANK_H          13
#define TANK_W          13
#define T_FRMS_N        2
#define S_FRMS_N        1

#define SHEL_W          1
#define SHEL_H          3

#define TANK_FRAME_S    (((TANK_H * TANK_W) | 15) + 1)

#define SHEL_FRAME_S    (((SHEL_H * SHEL_W) | 15) + 1)

extern const uint16_t tank_ally_frames[SIDES][T_FRMS_N][TANK_FRAME_S];

extern const uint16_t tank_enemy_frames[SIDES][T_FRMS_N][TANK_FRAME_S];

extern const uint16_t shell_frames[SHEL_FRAME_S];

extern const uint16_t img_clear[TANK_FRAME_S];

/* Boot-clear strips of the display drivers (see assets.cpp): the
 * driver instantly takes its address as CLEAR_IMG (clear of the boot
 * segment, sudo on its own panel). Flash-resident all-zero strips;
 * each platform defines only the strip its instantiation consumes. */
extern const uint16_t disp_clear_240x16[240 * 16];
extern const uint16_t disp_clear_160x16[160 * 16];