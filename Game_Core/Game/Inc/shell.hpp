#pragma once

#include <assets.hpp>
#include "map.hpp"
#include "entity.hpp"

#define SHELL_LENGTH 3

/* The projectile of one Game: no own display fantasy, the blit path
 * runs through the typed driver of the owning Entity<LcdT>. */
template <class LcdT>
class Shell final : public Entity<LcdT> {
public:
  /* Re-export the dependent base members into this scope: hybrid by
   * usage census like tank.hpp. The round-trips stay bare - x/y/speed
   * (4 sites each); the rare clear (2), frame (2), dir, h,
   * updateImageWH (1 each) go through this->. In get_out() the this->
   * form is the only way anyway - the params shadow x/y/side/dir. */
  using Entity<LcdT>::x;
  using Entity<LcdT>::y;
  using Entity<LcdT>::speed;

  bool life = true;

  Shell();

  Shell(int x, int y, int dir, int side);

  void move(int fld_x_end, int fld_y_end, MapT<LcdT> &map);
  void update(int fld_x_end, int fld_y_end, MapT<LcdT> &map);

  void get_out(int x, int y, int dir, int side);
  void explode();
};

template <class LcdT>
Shell<LcdT>::Shell() : Entity<LcdT>(0, 0, 1, 3, 3, 1, MOVE, 1)
{
  this->frame = &shell_frames[0];
}

template <class LcdT>
Shell<LcdT>::Shell(const int x, const int y, const int dir, const int side) : Entity<LcdT>(x, y, 1, 3, 3, dir, MOVE, side)
{
  this->frame = &shell_frames[0];
}

template <class LcdT>
void Shell<LcdT>::move(const int fld_x_end, const int fld_y_end, MapT<LcdT>& map)
{
  switch (this->dir) {
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

  const int l_hlf = this->h / 2;

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

  /* The map: steel kills the shell, water lets it pass, the brick
   * cells under the hull are blown away (marked; the draw phase
   * repaints them) and kill it. The hull is the oriented form
   * (w_img/h_img) - the stored w/h is the vertical one only, the
   * horizontal flight is 3x1. */
  const int wr = this->w_img / 2;
  const int hr = this->h_img / 2;

  if (map.hit(x - wr, y - hr, this->w_img, this->h_img))
    life = false;
}

template <class LcdT>
void Shell<LcdT>::update(const int fld_x_end, const int fld_y_end, MapT<LcdT>& map)
{
  move(fld_x_end, fld_y_end, map);

  if (!life)
    this->clear();
}

template <class LcdT>
void Shell<LcdT>::get_out(const int x, const int y, const int dir, const int side)
{
  this->x = x;
  this->y = y;
  life = true;
  this->side = side;
  this->dir = dir;
  this->updateImageWH();
}

template <class LcdT>
void Shell<LcdT>::explode()
{
  life = false;

  this->clear();
}
