#include <math.h>
#include "tank.hpp"
#include "display.hpp"
#include <assets.hpp>
#include "shell.hpp"
#include "config.hpp"


Tank::Tank() :
  Entity(0, 0, 13, 13, 1, 0, KIL, 0), frame_buff(tank_ally_frames)
{
  x_shot = 0;
  y_shot = 0;
  frame_buff_directed = frame_buff[this->dir];
  frame_i = 0;
  frame = &frame_buff_directed[0][0];
}

Tank::Tank(const int x, const int y, const int dir,const int side) :
  Entity(x, y, 13, 13, 1, dir, KIL, side), frame_buff((side == 0) ? tank_ally_frames : tank_enemy_frames)
{
  x_shot = x;
  y_shot = y;
  frame_buff_directed = frame_buff[this->dir];
  frame_i = 0;
  frame = &frame_buff_directed[frame_i][0];
}

void Tank::setSide(const int side)
{
  this->side = side;
  frame_buff = (side == 0) ? tank_ally_frames : tank_enemy_frames;
  frame_buff_directed = frame_buff[dir];
  frame_i = 0;
  frame = &frame_buff_directed[frame_i][0];
}

void Tank::move(const int fld_x_end, const int fld_y_end)
{
  action = MOVE;
  ch_dir_cd += 1;
  
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

void Tank::back(const bool stuck)
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

void Tank::rotate(const int new_dir = false)
{
  dir = (new_dir % 4);
  this->frame_buff_directed = frame_buff[dir];
  action = ROTATE;
  //updateImgWH();
  frame = &frame_buff_directed[frame_i][0];
}

void Tank::update(const int fld_x_end, const int fld_y_end)
{
  if ((ch_dir_cd > 70) || (stuck_cnt > 3)) {
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
  
  if (shot_cd >= 60) {
    if ((rand() / 4096) > 2200) {
      shot_cd = 0;
      shot_st = true;
    }
    else
      shot_cd >>= 1;
  }
  else
    shot_cd += 1;
}

bool Tank::isShot()
{
  if (!shot_st)
    return false;

  shot_st = false;

  x_shot = x;
  y_shot = y;

  switch (dir) {
    case UP:
      y_shot -= (h / 2);
      break;
    case DOWN:
      y_shot += (h / 2);
      break;
    case LEFT:
      x_shot -= (w / 2);
      break;
    case RIGHT:
      x_shot += (w / 2);
      break;
    default: break;
  }

  if (x_shot <= SHELL_LENGTH / 2)
    return false;
  if (x_shot >= FIELD_WIDTH - (SHELL_LENGTH / 2))
    return false;

  if (y_shot <= SHELL_LENGTH / 2)
    return false;
  if (y_shot >= FIELD_HEIGHT - (SHELL_LENGTH / 2))
    return false;

  return true;
}

void Tank::shot(Shell &shell) const
{
  shell.get_out(x_shot, y_shot, dir, side);
}

void Tank::respawn(const int x, const int y, const int side)
{
  life = 2;
  this->x = x;
  this->y = y;
  shot_cd = 0;
  this->action = ROTATE;
  dir = (side) ? 3 : 1;
  frame_i = 0;
  ch_dir_cd = 0;
  stuck_cnt = 0;

  //updateImgWH();
  setSide(side);
}

bool Tank::kill() const
{
  if (life > 0)
    return false;

  clear();
  return true;
}

Tank::~Tank()
{
  clear();
}
