#include "entity.hpp"
#include "display.hpp"
#include "assets.hpp"

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
  const uint16_t w_r = w_img / 2;
  const uint16_t h_r = h_img / 2;

  if (action == MOVE)
  {
    switch (dir) {
      case UP:
        display_print_img(x - w_r, y + (h_r + 1), w_img, speed, img_clear);
        break;
      case DOWN:
        display_print_img(x - w_r, y - (h_r + speed), w_img, speed, img_clear);
        break;
      case LEFT:
        display_print_img(x + (w_r + 1), y - h_r, speed, h_img, img_clear);
        break;
      case RIGHT:
        display_print_img(x - (w_r + speed), y - h_r, speed, h_img, img_clear);
        break;
      default: break;
    }
  }

  display_print_img(x - w_r, y - h_r, w_img, h_img, frame);
}

void Entity::clear() const
{
  const uint16_t w_r = w_img / 2;
  const uint16_t h_r = h_img / 2;

  int x_t = this->x;
  int y_t = this->y;

  switch (dir) {
    case UP:
      y_t += speed;
      break;
    case DOWN:
      y_t -= speed;
      break;
    case LEFT:
      x_t += speed;
      break;
    case RIGHT:
      x_t -= speed;
      break;
    default: break;
  }
  display_print_img(x_t - w_r, y_t - h_r, w_img, h_img, img_clear);
}

Entity::~Entity() = default;
