#include "math.h"
#include "game.hpp"

#include "config.hpp"
#include "display.hpp"
#include "tank.hpp"
#include "shell.hpp"


Game::Game()
{
  fld_x_end = FIELD_HEIGHT - 1;
  fld_y_end = FIELD_WIDTH - 1;

  display_init();
}

bool Game::is_booked(const int &spawn_x, const int &spawn_y, const Tank &tank)
{
  const int dist_x = tank.w - 1 + 2;
  const int dist_y = tank.h - 1 + 2;

  if ((fabs(spawn_x - tank.x) >= dist_x) || (fabs(spawn_y - tank.y) >= dist_y))
    return false;

  return true;
}

void Game::spawn()
{
  for (int s_i = 0; s_i < 2; ++s_i)
  {
    const int units_spawn = unit_n_max[s_i] - unit_n_cur[s_i];

    for (int u_i = 0; u_i < units_spawn; ++u_i)
    {
      int x = (s_i == 0) ? 30 : 210;
      int y = 30 * (u_i + 1);

      int booked = false;

      for (int t_i = 0; t_i < tanks.active_n(); ++t_i)
      {
        if (!is_booked(x, y, *tanks[t_i]))
          continue;

        booked = true;
        break;
      }
      if (booked)
        continue;

      Tank* activated = tanks.sleep();

      activated->respawn(x, y, s_i);
      tanks.activate();

      unit_n_cur[s_i] += 1;
    }
  }
}

void Game::resolve_collision(Tank &t_moved, const Tank &t_stayed)
{
  const int dist_x = (t_moved.w + t_stayed.w) / 2;
  const int dist_y = (t_moved.h + t_stayed.h) / 2;

  if ((fabs(t_moved.x - t_stayed.x) >= dist_x) || (fabs(t_moved.y - t_stayed.y) >= dist_y))
    return;
  
  t_moved.back(true);
}

void Game::resolve_collision(Shell &shell, Tank &tank)
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

  unit_n_cur[tank.side] -= (tank.kill()) ? 1 : 0;
}

void game_init()
{
  display_init();
}

void Game::loop()
{
  static int game_i = 0;
  ++game_i;

  if (game_i > 9 * 5)
    game_i = 0;

  spawn();

  for (int t_i = 0; t_i < tanks.active_n(); ++t_i)
  {
    tanks[t_i]->update(fld_x_end, fld_y_end);
    for (int t_c = 0; t_c < tanks.active_n(); ++t_c)
    {
      if (t_c == t_i) continue;

      resolve_collision(*tanks[t_i], *tanks[t_c]);
    }
    if (!tanks[t_i]->isShot())
      continue;

    Shell* shell_ready = shells.activate();
    tanks[t_i]->shot(*shell_ready);
  }

  for (int s_i = 0; s_i < shells.active_n(); ++s_i)
  {
    if (!(shells[s_i]->life)) continue;

    shells[s_i]->update(fld_x_end,fld_y_end);

    for (int t_i = 0; t_i < tanks.active_n(); ++t_i)
      resolve_collision(*shells[s_i], *tanks[t_i]);
  }

  for (const Shell* shell = shells.start(); shells.is_nEnd(); shell = shells.next())
    if (!(shell->life)) shells.lull();


  for (const Tank* tank = tanks.start(); tanks.is_nEnd(); tank = tanks.next())
    if (tank->life < 1) tanks.lull();

  for (int t_i = 0; t_i < tanks.active_n(); ++t_i)
    tanks[t_i]->print();

  for (int s_i = 0; s_i < shells.active_n(); ++s_i)
    shells[s_i]->print();
}