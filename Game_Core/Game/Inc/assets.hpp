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

extern uint16_t tank_ally_frames[SIDES][T_FRMS_N][TANK_FRAME_S];

extern uint16_t tank_enemy_frames[SIDES][T_FRMS_N][TANK_FRAME_S];

extern uint16_t shell_frames[SHEL_FRAME_S];

