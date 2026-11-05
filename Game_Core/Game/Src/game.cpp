/* abs(int) for the collision math; no floating point left in the Game. */
#include <cstdlib>
#include "game.hpp"

#include "config.hpp"
#include "tank.hpp"
#include "shell.hpp"


/* The display driver of this platform; injected in the ctor, applied in init(). */
Game::Game(Disp_Iface* d) : disp(d) {}

void Game::init()
{
  disp->init();

  /* The panel geometry is known after init(): the field size is taken
   * from the real width()/height() of the connected matrix. */
  fld_x_end = disp->w() - 1;
  fld_y_end = disp->h() - 1;

  /* Spawn config is derived from the real field: two columns pinned
   * cfg::spawn_margin_px from the field edges, the slots spread evenly
   * over the height. For a 240px panel this reproduces the old literal
   * ladder 30..210 by 30. */
  const int W = disp->w();
  const int span = disp->h() - 2 * cfg::spawn_margin_px;
  for (int s_i = 0; s_i < 2; ++s_i)
    for (int s = 0; s < cfg::spawn_slots; ++s)
      spawn_coords[s_i][s] = {
          .x = (s_i == 0) ? cfg::spawn_margin_px : (W - cfg::spawn_margin_px),
          .y = cfg::spawn_margin_px + (s * span) / (cfg::spawn_slots - 1),
      };

  /* Stamp every pool slot of this Game with its own display. Pool
   * entities are realized by reuse (respawn/get_out), not by
   * re-construction - one stamp here covers the whole game life. */
  tanks.set_disp(disp);
  shells.set_disp(disp);
}

bool Game::is_booked(const point_t &pt, const Tank &tank)
{
  const int dist_x = tank.w - 1 + 2;
  const int dist_y = tank.h - 1 + 2;

  if ((abs(pt.x - tank.x) >= dist_x) || (abs(pt.y - tank.y) >= dist_y))
    return false;

  return true;
}

void Game::spawn()
{
  for (int s_i = 0; s_i < 2; ++s_i)
    /* Linear scan of the side's slots: spawn into the first free
     * ones until the side's quota is met. Booked slots are retried
     * on the following frames. */
    for (int s = 0; s < cfg::spawn_slots && unit_n_cur[s_i] < unit_n_max[s_i]; ++s)
    {
      const point_t pt = spawn_coords[s_i][s];

      bool booked = false;

      for (int t_i = 0; t_i < tanks.active_n(); ++t_i)
      {
        if (!is_booked(pt, *tanks[t_i]))
          continue;

        booked = true;
        break;
      }
      if (booked)
        continue;

      Tank* free_tank = tanks.sleep();

      if (!free_tank)
        return;

      free_tank->respawn(pt.x, pt.y, s_i);
      tanks.activate();

      unit_n_cur[s_i] += 1;
    }
}

void Game::resolve_collision(Tank &t_moved, const Tank &t_stayed)
{
  const int dist_x = (t_moved.w + t_stayed.w) / 2;
  const int dist_y = (t_moved.h + t_stayed.h) / 2;

  if ((abs(t_moved.x - t_stayed.x) >= dist_x) || (abs(t_moved.y - t_stayed.y) >= dist_y))
    return;
  
  t_moved.back(true);
}

void Game::resolve_collision(Shell &shell, Tank &tank)
{
  const int dist_x = (tank.w + shell.w) / 2;
  const int dist_y = (tank.h + shell.h) / 2;

  if ((abs(tank.x - shell.x) >= dist_x) || (abs(tank.y - shell.y) >= dist_y))
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

void Game::loop()
{
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
    if (!tanks[t_i]->isShot(fld_x_end, fld_y_end))
      continue;

    /* Full pool: this shot doesn't happen on this frame. */
    Shell* shell_ready = shells.activate();

    if (!shell_ready)
      continue;

    tanks[t_i]->shot(*shell_ready);
  }

  /* Drawing cycle opens here (the tank phase above doesn't draw).
   * MAINTENANCE: new blits above this line - move select() above
   * them, on a shared bus anything printed before the pick is
   * ignored (CS high). */
  disp->select();

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

  disp->unselect();
}