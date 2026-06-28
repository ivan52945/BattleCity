#include "math.h"
#include "game.hpp"
#include "lobby.hpp"
#include "display.hpp"
#include "point.hpp"
#include "tank.hpp"
#include "shell.hpp"

#define TANKS_N 3

static Tank tanks_init[TANKS_N * 2]  = {
  Tank(0,0,0, 0),
  Tank(0,0,0, 0),
  Tank(0,0,0, 0),
  //Tank(0,0,0, 0),
  //Tank(0,0,0, 0),
  //Tank(0,0,0, 0),
  Tank(0,0,0, 1),
  Tank(0,0,0, 1),
  Tank(0,0,0, 1),
  //Tank(0,0,0, 1),
  //Tank(0,0,0, 1),
  //Tank(0,0,0, 1),
};

static Shell shells_init[200] = { };

static Lobby<Tank, TANKS_N * 2> tanks(tanks_init, TANKS_N * 2);
static Lobby<Shell, 96> shells(shells_init, 48);

bool is_booked(const point_t &point, const Tank &tank)
{
  const int dist_x = tank.w - 1 + 2;
  const int dist_y = tank.h - 1 + 2;

  if ((fabs(point.x - tank.x) >= dist_x) || (fabs(point.y - tank.y) >= dist_y))
    return false;

  return true;
}

void spawn()
{
  int p_i[2] = {0, 0};
  const int n_sleep = tanks.sleep_n();

  for (int i_sleep = 0; i_sleep < n_sleep; ++i_sleep)
  {

    Tank* tank_ready = tanks.sleep();
    if (!tank_ready)
      break;

    bool is_free = true;
    const int side = tank_ready->side;

    for (const Tank* tank_active = tanks.start(); tank_active != nullptr; tank_active = tanks.next())
    {
      if (!is_booked(tank_spawns[side][p_i[side]], *tank_active))
        continue;

      is_free = false;
      break;
    }

    if (is_free)
    {
      tank_ready->respawn(tank_spawns[side][p_i[side]].x, tank_spawns[side][p_i[side]].y);
      tanks.activate();
    }
    p_i[side] += 1;
  }
}

void resolve_collision(Tank &t_moved, const Tank &t_stayed)
{
  const int dist_x = (t_moved.w + t_stayed.w) / 2;
  const int dist_y = (t_moved.h + t_stayed.h) / 2;

  if ((fabs(t_moved.x - t_stayed.x) >= dist_x) || (fabs(t_moved.y - t_stayed.y) >= dist_y))
    return;
  
  t_moved.back(true);
}

void resolve_collision(Shell &shell, Tank &tank)
{
  const int dist_x = (tank.w + shell.w) / 2;
  const int dist_y = (tank.h + shell.h) / 2;

  if ((fabs(tank.x - shell.x) >= dist_x) || (fabs(tank.y - shell.y) >= dist_y))
    return;

  shell.explode();

  if (shell.side == tank.side)
    return;
  
  if (abs(shell.dir - tank.dir) == 2)
    tank.life -= 1;
  else
    tank.life -= 2;

  tank.kill();
}

void game_init()
{
  display_init();
}

void game_loop()
{
  static int game_i = 0;
  ++game_i;

  if (game_i > 9*5)
    game_i = 0;

  spawn();

  for (Tank** t_upd = tanks.r_start(); *t_upd != nullptr; ++t_upd)
  {
    (*t_upd)->update();
    for (Tank** t_check = tanks.r_start(); *t_check != nullptr; ++t_check)
    {
      if (*t_check == *t_upd) continue;

      resolve_collision(**t_upd, **t_check);
    }
    if (!(*t_upd)->isShot())
      continue;

    Shell* shell_ready = shells.activate();
    (*t_upd)->shot(*shell_ready);
  }

  for (Shell** shell = shells.r_start(); *shell != nullptr; ++shell)
  {
    if (!((*shell)->life)) continue;

    (*shell)->update();

    for (Tank** tank = tanks.r_start(); *tank != nullptr; ++tank)
      resolve_collision(**shell, **tank);
  }

  for (const Shell* shell = shells.start(); shell != nullptr; shell = shells.next())
    if (!(shell->life)) shells.lull();


  for (const Tank* tank = tanks.start(); tank != nullptr; tank = tanks.next())
    if (tank->life < 1) tanks.lull();

  for (Tank** tank = tanks.r_start(); *tank != nullptr; ++tank)
    (*tank)->print();

  for (Shell** shell = shells.r_start(); *shell != nullptr; ++shell)
    (*shell)->print();
}