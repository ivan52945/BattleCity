#pragma once

#include "assets.hpp"
#include "entity.hpp"

#define SHELL_LENGTH 3

class Shell: public Entity {
public:

  bool life = true;

  Shell();

  Shell(int x, int y, int dir, int side);

  void move(int fld_x_end, int fld_y_end);
  void update(int fld_x_end, int fld_y_end);

  void get_out(int x, int y, int dir, int side);
  void explode();

  ~Shell();
};
