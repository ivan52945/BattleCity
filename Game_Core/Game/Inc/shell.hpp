#pragma once

#include "assets.hpp"
#include "entity.hpp"
#include "config.hpp"

#define SHELL_LENGTH 3

class Shell: public Entity {
public:

  bool life = true;

  Shell();

  Shell(int x, int y, int dir, int side);

  void move();
  void update();

  void get_out(int x, int y, int dir, int side);
  void explode();

  ~Shell();
};
