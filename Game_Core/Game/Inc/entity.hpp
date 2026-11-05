#pragma once

#include "stdint.h"

/* The docking point: the abstract display contract of the Game Core.
 * The concrete driver class stays unknown here. */
#include "disp_iface.hpp"

/* Direction of an entity: which way it looks/moves on the panel.
 * UP(0) vs DOWN(2), RIGHT(1) vs LEFT(3) - opposite pairs for
 * the `(dir ± 2)` collision logic. */
enum side_t {
  UP, RIGHT, DOWN, LEFT,
};

enum act_t {
  KIL, ROTATE, MOVE, BLOCK,
};

class Entity
{
public:
  int x;
  int y;
  int dir;
  int side;
  const int w;
  /* Points to flash-resident sprite data (const arrays in assets.cpp). */
  const uint16_t* frame;
  const int speed;
  const int h;
  int w_img;
  int h_img;
  act_t action;

  /* The display of the owning Game: each pool slot carries its own
   * pointer, so several Game instances can co-exist, each on its own
   * panel. Stamped once in Game::init() over the whole pool: the
   * pools realize entities by reusing slots (respawn/get_out), never
   * re-constructing them - one stamp covers the slot's whole life
   * and still lands before the first print()/clear() can run. */
  Disp_Iface* disp = nullptr;

  Entity(int x, int y, int w, int h, int speed, int dir, act_t act, int side);

  void updateImageWH();

  void print() const;

  void clear() const;

  ~Entity();
};
