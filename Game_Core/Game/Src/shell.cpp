#include "shell.hpp"
#include "display.hpp"
#include "assets.hpp"

Shell::Shell() : Entity(0, 0, 1, 3, 3, 1, MOVE, 1)
{
  frame = &shell_frames[0];
}

Shell::Shell(const int x, const int y, const int dir, const int side) : Entity(x, y, 1, 3, 3, dir, MOVE, side)
{
  frame = &shell_frames[0];
}

void Shell::move(const int fld_x_end, const int fld_y_end)
{
  switch (dir) {
    case UP:
      y -= speed;
      break;
    case DOWN:
      y += speed;
      break;
    case LEFT:
      x -= speed;
      break;
    case RIGHT:
      x += speed;
      break;
    default:
      break;
  }

  const int l_hlf = h / 2;

  if (x < l_hlf) {
    life = false;
  }
  else if (x > (fld_x_end - l_hlf)) {
    life = false;
  }

  if (y < l_hlf) {
    life = false;
  }
  else if (y > (fld_y_end - l_hlf)) {
    life = false;
  }
}

void Shell::update(const int fld_x_end, const int fld_y_end)
{
  move(fld_x_end, fld_y_end);

  if (!life)
    clear();
}

void Shell::get_out(const int x, const int y, const int dir, const int side)
{
  this->x = x;
  this->y = y;
  life = true;
  this->side = side;
  this->dir = dir;
  updateImageWH();
}

void Shell::explode()
{
  life = false;

  clear();
}

Shell::~Shell()
{
  clear();
}
