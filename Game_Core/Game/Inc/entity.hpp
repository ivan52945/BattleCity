#pragma once

#include "stdint.h"

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
  uint16_t* frame;
  const int speed;
  const int h;
  int w_img;
  int h_img;
  act_t action;

  Entity(int x, int y, int w, int h, int speed, int dir, act_t act, int side);

  void updateImageWH();

  void print() const;

  void clear() const;

  ~Entity();
};
