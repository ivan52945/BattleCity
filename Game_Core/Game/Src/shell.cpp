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

void Shell::move()
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


  if (x < 1) {
    life = false;
  }
  else if (x > (FIELD_WIDTH - 1 - w / 2)) {
    life = false;
  }

  if (y < 1) {
    life = false;
  }
  else if (y > (FIELD_HEIGHT - 1 - (h / 2))) {
    life = false;
  }
}

void Shell::update()
{
  move();

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
