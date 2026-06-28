#include "entity.hpp"
#include "display.hpp"

Entity::Entity(const int x, const int y, const int w, const int h, const int speed, const int dir, const act_t act, const int side) : w(w), speed(speed), h(h), action(act)
{
  this->x = x;
  this->y = y;
  this->side = side;
  this->dir = dir;
  updateImageWH();
}

void Entity::updateImageWH()
{
  switch (dir) {
    case UP:
    case DOWN:
      w_img = w;
      h_img = h;
      break;
    case LEFT:
    case RIGHT:
      w_img = h;
      h_img = w;
      break;
    default:
      break;
  }
}

void Entity::print() const
{
  print_moved_img(x, y, w_img, h_img, (action == MOVE), dir, speed, frame);
}

void Entity::clear() const
{
  print_moved_clr(x, y, w_img, h_img, (action == MOVE), dir, speed);
}

Entity::~Entity() = default;
