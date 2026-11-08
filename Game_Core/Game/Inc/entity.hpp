#pragma once

#include <stdint.h>

/* Sprite data of the pool: flash-resident const arrays (assets.cpp). */
#include <assets.hpp>

/* Direction of an entity: which way it looks/moves on the panel.
 * UP(0) vs DOWN(2), RIGHT(1) vs LEFT(3) - opposite pairs for
 * the `(dir ± 2)` collision logic. */
enum side_t {
  UP, RIGHT, DOWN, LEFT,
};

enum act_t {
  KIL, ROTATE, MOVE, BLOCK,
};

/* The docking point: the Game Core is typed against the concrete
 * (final) driver class. The blit path calls the driver's static
 * print without any object or pointer - the transaction chain of
 * the wiring carries no per-object state, so the panel identity lives
 * in the type alone. Lifecycle (init/select/unselect) goes to the one
 * driver object held by the Game. No display header is included
 * here: the type travels with the Game into main(). */
template <class LcdT>
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

  Entity(int x, int y, int w, int h, int speed, int dir, act_t act, int side);

  void updateImageWH();

  void print() const;

  void clear() const;

  ~Entity();
};

template <class LcdT>
Entity<LcdT>::Entity(const int x, const int y, const int w, const int h, const int speed, const int dir, const act_t act, const int side) : w(w), speed(speed), h(h), action(act)
{
  this->x = x;
  this->y = y;
  this->side = side;
  this->dir = dir;
  updateImageWH();
}

template <class LcdT>
void Entity<LcdT>::updateImageWH()
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

template <class LcdT>
void Entity<LcdT>::print() const
{
  const uint16_t w_r = w_img / 2;
  const uint16_t h_r = h_img / 2;

  if (action == MOVE)
  {
    switch (dir) {
      case UP:
        LcdT::print(x - w_r, y + (h_r + 1), w_img, speed, img_clear);
        break;
      case DOWN:
        LcdT::print(x - w_r, y - (h_r + speed), w_img, speed, img_clear);
        break;
      case LEFT:
        LcdT::print(x + (w_r + 1), y - h_r, speed, h_img, img_clear);
        break;
      case RIGHT:
        LcdT::print(x - (w_r + speed), y - h_r, speed, h_img, img_clear);
        break;
      default: break;
    }
  }

  LcdT::print(x - w_r, y - h_r, w_img, h_img, frame);
}

template <class LcdT>
void Entity<LcdT>::clear() const
{
  /* Erase the rect where the screen actually holds the sprite. The
   * caller keeps action true to this frame's net displacement: MOVE
   * = the full step went, print() last frame drew the sprite speed
   * px against dir of here - the shift walks back to it. BLOCK
   * (edge clamp, tank-tank bounce) = zero net move, the sprite is
   * still at x/y. A clear placed by the wrong branch wipes one rect
   * off: the far edge row/column stays as a 13x1 ghost line no later
   * pass ever visits - the wall-pressed deaths used to bequeath it
   * to the field rim. Shells always walk the MOVE branch: their
   * frame always holds the full speed, their action never leaves
   * MOVE. */
  const uint16_t w_r = w_img / 2;
  const uint16_t h_r = h_img / 2;

  int x_t = x;
  int y_t = y;
  if (action == MOVE)
  {
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
  }

  LcdT::print(x_t - w_r, y_t - h_r, w_img, h_img, img_clear);
}

template <class LcdT>
Entity<LcdT>::~Entity() = default;
