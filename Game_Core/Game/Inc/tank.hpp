#pragma once

#include <assets.hpp>
#include "shell.hpp"
#include "entity.hpp"
#include <cstdlib> /* rand */

#define TANK_WIDTH 13
#define TANK_HEIGHT 13

/* The tank of one Game, typed against the driver of that Game. The
 * frames are the flash-resident const arrays of assets.cpp
 * (ally/enemy sets), the pointer to the active frame travels through
 * the pools as data - no re-construction on respawn. */
template <class LcdT>
class Tank final : public Entity<LcdT> {
private:
  int x_shot;
  int y_shot;
  int frame_i;
  const uint16_t (*frame_buff_directed)[TANK_FRAME_S];
  const uint16_t (*frame_buff)[T_FRMS_N][TANK_FRAME_S];
  int shot_cd = 0;
  int ch_dir_cd = 0;
  int stuck_cnt = 0;
public:
  /* Re-export the dependent base members into this scope: the members
   * of Entity<LcdT> are visible only through the dependent base in a
   * template - a using keeps the method bodies as plain as the old
   * tank.cpp had them. Hybrid by usage census (bare sites per name):
   * the six round-trips stay bare - dir 10, x/y 9, speed 8, action 8,
   * frame 5 - plus side (1) kept bare by preference; the rest (h 3,
   * w 2, clear 1) go through this-> instead of a canalisation line. */
  using Entity<LcdT>::x;
  using Entity<LcdT>::y;
  using Entity<LcdT>::dir;
  using Entity<LcdT>::action;
  using Entity<LcdT>::side;
  using Entity<LcdT>::speed;
  using Entity<LcdT>::frame;

  int life = 2;
  bool shot_st = false;

  Tank();

  Tank(int x, int y, int dir, int side);

  void move(int fld_x_end, int fld_y_end);
  void update(int fld_x_end, int fld_y_end);
  bool isShot(const int fld_x_end, const int fld_y_end);
  void back(bool stuck = false);
  void shot(Shell<LcdT> &shell) const;
  void setSide(int side);
  bool kill() const;
  void rotate(int new_dir = 0);
  void respawn(int x, int y, int side);
};

template <class LcdT>
Tank<LcdT>::Tank() :
  Entity<LcdT>(0, 0, 13, 13, 1, 0, KIL, 0), frame_buff(tank_ally_frames)
{
  x_shot = 0;
  y_shot = 0;
  frame_buff_directed = frame_buff[dir];
  frame_i = 0;
  frame = &frame_buff_directed[0][0];
}

template <class LcdT>
Tank<LcdT>::Tank(const int x, const int y, const int dir,const int side) :
  Entity<LcdT>(x, y, 13, 13, 1, dir, KIL, side), frame_buff((side == 0) ? tank_ally_frames : tank_enemy_frames)
{
  x_shot = x;
  y_shot = y;
  frame_buff_directed = frame_buff[this->dir];
  frame_i = 0;
  frame = &frame_buff_directed[frame_i][0];
}

template <class LcdT>
void Tank<LcdT>::setSide(const int side)
{
  this->side = side;
  frame_buff = (side == 0) ? tank_ally_frames : tank_enemy_frames;
  frame_buff_directed = frame_buff[dir];
  frame_i = 0;
  frame = &frame_buff_directed[frame_i][0];
}

template <class LcdT>
void Tank<LcdT>::move(const int fld_x_end, const int fld_y_end)
{
  action = MOVE;
  /* ch_dir_cd here is NOT incremented: the direction-pump cooldown has
   * a single accounting site in update() (+1 per frame). */
  
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

  const int l_hlf = this->h / 2;

  if (x < l_hlf) {
    x = l_hlf;
    action = BLOCK;
    stuck_cnt += 1;
  }
  else if (x > (fld_x_end - l_hlf)) {
    x = fld_x_end - l_hlf;
    action = BLOCK;
    stuck_cnt += 1;
  }
  
  if (y < l_hlf) {
    y = l_hlf;
    action = BLOCK;
    stuck_cnt += 1;
  }
  else if (y > (fld_y_end - l_hlf)) {
    stuck_cnt += 1;
    y = fld_y_end - l_hlf;
    action = BLOCK;
  }
  
  frame_i = (frame_i + 1) % T_FRMS_N;
  frame = &frame_buff_directed[frame_i][0];
}

template <class LcdT>
void Tank<LcdT>::back(const bool stuck)
{
  switch (dir) {
    case UP:
      y += speed;
      break;
    case DOWN:
      y -= speed;
      break;
    case LEFT:
      x += speed;
      break;
    case RIGHT:
      x -= speed;
      break;
    default:
      break;
  }
  
  if (stuck) {
    stuck_cnt += (stuck_cnt < 10) ? 1 : 0;
    action = BLOCK;
  }
  //This function don't  use print
}

template <class LcdT>
void Tank<LcdT>::rotate(const int new_dir)
{
  dir = (new_dir % 4);
  frame_buff_directed = frame_buff[dir];
  action = ROTATE;
  //updateImgWH();
  frame = &frame_buff_directed[frame_i][0];
}

template <class LcdT>
void Tank<LcdT>::update(const int fld_x_end, const int fld_y_end)
{
  /* Direction pump: every 35 honest frames (ch_dir_cd counts +1 per
   * frame, a single accounting site) roll ~36%. Hit - rotate.
   * Miss - the cooldown halves, so the tank retries roughly twice as
   * soon (~18 frames): pressure keeps growing while the roll keeps
   * failing. */
  if ((ch_dir_cd > 35) || (stuck_cnt > 3)) {
    if ((rand() % 4096) > 2600) {
      stuck_cnt = 0;
      ch_dir_cd = 0;
      const int new_dir = dir + (rand() % 3) + 1;
      rotate(new_dir);
    }
    else
      ch_dir_cd >>= 1;
  }
  else
    ch_dir_cd += 1;
  
  move(fld_x_end, fld_y_end);
  
  /* Fire roll: honestly probabilistic (was rand()/4096 - a typo that
   * made the roll ~100% true, i.e. plated periodic fire). After the
   * 60-frame cooldown: ~46% to open fire. Miss halves shot_cd, so the
   * next roll comes ~30 frames later. */
  if (shot_cd >= 60) {
    if ((rand() % 4096) > 2200) {
      shot_cd = 0;
      shot_st = true;
    }
    else
      shot_cd >>= 1;
  }
  else
    shot_cd += 1;
}

template <class LcdT>
bool Tank<LcdT>::isShot(const int fld_x_end, const int fld_y_end)
{
  if (!shot_st)
    return false;

  shot_st = false;

  x_shot = x;
  y_shot = y;

  switch (dir) {
    case UP:
      y_shot -= (this->h / 2);
      break;
    case DOWN:
      y_shot += (this->h / 2);
      break;
    case LEFT:
      x_shot -= (this->w / 2);
      break;
    case RIGHT:
      x_shot += (this->w / 2);
      break;
    default: break;
  }

  if (x_shot <= SHELL_LENGTH / 2)
    return false;
  if (x_shot >= fld_x_end + 1 - (SHELL_LENGTH / 2))
    return false;

  if (y_shot <= SHELL_LENGTH / 2)
    return false;
  if (y_shot >= fld_y_end + 1 - (SHELL_LENGTH / 2))
    return false;

  return true;
}

template <class LcdT>
void Tank<LcdT>::shot(Shell<LcdT> &shell) const
{
  shell.get_out(x_shot, y_shot, dir, side);
}

template <class LcdT>
void Tank<LcdT>::respawn(const int x, const int y, const int side)
{
  life = 2;
  this->x = x;
  this->y = y;
  shot_cd = 0;
  action = ROTATE;
  dir = (side) ? 3 : 1;
  frame_i = 0;
  ch_dir_cd = 0;
  stuck_cnt = 0;

  //updateImgWH();
  setSide(side);
}

template <class LcdT>
bool Tank<LcdT>::kill() const
{
  if (life > 0)
    return false;

  this->clear();

  return true;
}
