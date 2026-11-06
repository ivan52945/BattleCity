#pragma once

#include <cstdlib>
#include <array>
#include "disp.hpp"
#include "lobby.hpp"
#include "point.hpp"
#include "tank.hpp"
#include "shell.hpp"
#include "config.hpp"

/* The Game template: one instance rides one concrete driver class.
 * The whole chain (GameT<LcdT> -> Lobby<Tank<LcdT>> -> Entity<LcdT> ->
 * LcdT) carries the panel in the type system; the blit path dials the
 * static driver method without any object, the lifecycle calls go to
 * the one driver object this Game holds a reference to. The
 * Disp<LcdT> concept checks the driver shape at this instantiation
 * point; several GameT over different driver types co-exist in one
 * firmware by separate instantiations. */
template <Disp LcdT>
class GameT {
public:
    int unit_n_cur[2] = {0, 0};
    /* Simultaneous tanks quota per side (gameplay config). */
    int unit_n_max[2] = {cfg::units_per_side, cfg::units_per_side};
    /* The panel geometry is a compile-time constant of the driver
     * instantiation (w()/h() are constexpr static members of the
     * type): the field edges fold at compile time - the loops pass
     * immediates, and the object carries neither the old ladder nor
     * the two field-end ints. */
    static constexpr int fld_x_end = LcdT::w() - 1;
    static constexpr int fld_y_end = LcdT::h() - 1;

    /* The spawn ladder as a compile-time constexpr table: two columns
     * pinned cfg::spawn_margin_px from the field edges, the slots
     * spread evenly over the height (240px panel: 30..210 by 30 - the
     * same ladder the old init() loop drew). Flash-resident, not on
     * the object. */
    static constexpr auto make_spawn_coords()
        -> std::array<std::array<point_t, cfg::spawn_slots>, 2>
    {
        constexpr int span = LcdT::h() - 2 * cfg::spawn_margin_px;
        std::array<std::array<point_t, cfg::spawn_slots>, 2> coords = {};
        for (int s_i = 0; s_i < 2; ++s_i)
            for (int s = 0; s < cfg::spawn_slots; ++s)
                coords[s_i][s] = {
                    .x = (s_i == 0) ? cfg::spawn_margin_px
                                    : (LcdT::w() - cfg::spawn_margin_px),
                    .y = cfg::spawn_margin_px
                         + (s * span) / (cfg::spawn_slots - 1),
                };
        return coords;
    }

    static constexpr std::array<std::array<point_t, cfg::spawn_slots>, 2>
        spawn_coords = make_spawn_coords();

    Lobby<Tank<LcdT>, 20> tanks;
    Lobby<Shell<LcdT>, 96> shells;

    GameT(LcdT& d) : disp(d) {}

    void init();

    void loop();
private:
    void spawn();

    /* The one driver object of the platform: lifecycle calls ride
     * this reference; the blit path needs no object at all (the
     * driver's print path is static, no per-object state). */
    LcdT& disp;

    /* Frame counter of this Game instance; per-instance pacing. */
    int game_i = 0;

    /* True if a live tank sits too close to the spawn point. */
    static bool is_booked(const point_t &pt, const Tank<LcdT> &tank);

    static void resolve_collision(Tank<LcdT> &t_moved, const Tank<LcdT> &t_stayed);

    void resolve_collision(Shell<LcdT> &shell, Tank<LcdT> &tank);
};

template <Disp LcdT>
void GameT<LcdT>::init()
{
  /* Lifecycle only: the field geometry and the spawn ladder are
   * compile-time constants of the driver instantiation now, there is
   * nothing left to compute here. */
  disp.init();
}

template <Disp LcdT>
bool GameT<LcdT>::is_booked(const point_t &pt, const Tank<LcdT> &tank)
{
  const int dist_x = tank.w - 1 + 2;
  const int dist_y = tank.h - 1 + 2;

  if ((abs(pt.x - tank.x) >= dist_x) || (abs(pt.y - tank.y) >= dist_y))
    return false;

  return true;
}

template <Disp LcdT>
void GameT<LcdT>::spawn()
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

      Tank<LcdT>* free_tank = tanks.sleep();

      if (!free_tank)
        return;

      free_tank->respawn(pt.x, pt.y, s_i);
      tanks.activate();

      unit_n_cur[s_i] += 1;
    }
}

template <Disp LcdT>
void GameT<LcdT>::resolve_collision(Tank<LcdT> &t_moved, const Tank<LcdT> &t_stayed)
{
  const int dist_x = (t_moved.w + t_stayed.w) / 2;
  const int dist_y = (t_moved.h + t_stayed.h) / 2;

  if ((abs(t_moved.x - t_stayed.x) >= dist_x) || (abs(t_moved.y - t_stayed.y) >= dist_y))
    return;
  
  t_moved.back(true);
}

template <Disp LcdT>
void GameT<LcdT>::resolve_collision(Shell<LcdT> &shell, Tank<LcdT> &tank)
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

template <Disp LcdT>
void GameT<LcdT>::loop()
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
    Shell<LcdT>* shell_ready = shells.activate();

    if (!shell_ready)
      continue;

    tanks[t_i]->shot(*shell_ready);
  }

  /* Drawing cycle opens here (the tank phase above doesn't draw).
   * MAINTENANCE: new blits above this line - move select() above
   * them, on a shared bus anything printed before the pick is
   * ignored (CS high). */
  disp.select();

  for (int s_i = 0; s_i < shells.active_n(); ++s_i)
  {
    if (!(shells[s_i]->life)) continue;

    shells[s_i]->update(fld_x_end,fld_y_end);

    for (int t_i = 0; t_i < tanks.active_n(); ++t_i)
      resolve_collision(*shells[s_i], *tanks[t_i]);
  }

  for (const Shell<LcdT>* shell = shells.start(); shells.is_nEnd(); shell = shells.next())
    if (!(shell->life)) shells.lull();


  for (const Tank<LcdT>* tank = tanks.start(); tanks.is_nEnd(); tank = tanks.next())
    if (tank->life < 1) tanks.lull();

  for (int t_i = 0; t_i < tanks.active_n(); ++t_i)
    tanks[t_i]->print();

  for (int s_i = 0; s_i < shells.active_n(); ++s_i)
    shells[s_i]->print();

  disp.unselect();
}
