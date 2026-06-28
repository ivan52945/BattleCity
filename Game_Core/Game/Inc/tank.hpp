#pragma once

#include "stdint.h"
#include "shell.hpp"
#include <assets.hpp>
#include "lobby.hpp"
#include "entity.hpp"

#define TANK_WIDTH 13
#define TANK_HEIGHT 13

class Tank final : public Entity {
private:
  int x_shot;
  int y_shot;
  int frame_i;
  int shot_cd = 0;
  uint16_t (*frame_buff_directed)[TANK_FRAME_S];
  uint16_t (*frame_buff)[T_FRMS_N][TANK_FRAME_S];
  int ch_dir_cd = 0;
  int stuck_cnt = 0;
public:
  int life = 2;
  bool shot_st = false;

  Tank(int x, int y, int dir, int side);

  void move();
  void update();
  bool isShot();
  void back(bool stuck = false);
  void shot(Shell &shell) const;
  void setSide(int side);
  void kill() const;
  void rotate(int new_dir);
  void respawn(int x, int y);

  ~Tank();
};

