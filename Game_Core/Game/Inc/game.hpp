#pragma once

#include <cstdlib>
#include <array>
#include "disp.hpp"
#include "lobby.hpp"
#include "point.hpp"
#include "tank.hpp"
#include "shell.hpp"
#include "map.hpp"
#include "config.hpp"

/** \brief The Game template: one instance rides one concrete driver
 *  class.
 *
 *  \tparam LcdT the driver type of this Game
 *
 *  The whole chain (GameT<LcdT> -> Lobby<Tank<LcdT>> -> Entity<LcdT> ->
 *  LcdT) carries the panel in the type system; the blit path dials the
 *  static driver method without any object, the lifecycle calls go to
 *  the one driver object this Game holds a reference to. The
 *  Disp<LcdT> concept checks the driver shape at this instantiation
 *  point; several GameT over different driver types co-exist in one
 *  firmware by separate instantiations. */
template <Disp LcdT>
class GameT {
public:
    /** \brief The field edges: a compile-time constant of the driver
     *  instantiation (w()/h() are constexpr static members of the
     *  type) - the loops pass immediates, and the object carries
     *  neither the old ladder nor the two field-end ints. */
    static constexpr int fld_x_end = LcdT::w() - 1;
    static constexpr int fld_y_end = LcdT::h() - 1;

    /** \brief The spawn ladder as a compile-time constexpr table: two
     *  columns pinned cfg::spawn_margin_px from the field edges, the
     *  slots spread evenly over the height (240px panel: 30..210 by
     *  30 - the same ladder the old init() loop drew).
     *  Flash-resident, not on the object.
     *
     *  \return the [2][spawn_slots] table of spawn points */
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

    /** \brief The tank pool, sized exactly by the single source of
     *  truth - the per-side quota, twice. The active count is capped
     *  at 2 x quota by the spawn guards, and corpse slots are culled
     *  in the closing passes of the same frame, so the pool never
     *  overflows: the sleep() null bail in spawn() is structurally
     *  unreachable and removed. */
    Lobby<Tank<LcdT>, 2 * cfg::units_per_side> tanks;

    /** \brief The shell pool. */
    Lobby<Shell<LcdT>, 96> shells;

    /** \brief The terrain of the field. */
    MapT<LcdT> map;

    /** \brief The authored map must leave every spawn hull of this
     *  instantiation on passable cells - a solid under the ladder
     *  would freeze a fresh tank in the wall. Folded at compile time
     *  (see MapT::spawn_free): a map edit that blocks a spawn is a
     *  build error of this platform.
     *
     *  \return true when the whole ladder is spawn-free */
    static constexpr bool ladder_free()
    {
        for (int s_i = 0; s_i < 2; ++s_i)
            for (int s = 0; s < cfg::spawn_slots; ++s)
                if (!MapT<LcdT>::spawn_free(spawn_coords[s_i][s].x,
                                            spawn_coords[s_i][s].y))
                    return false;

        return true;
    }

    static_assert(ladder_free(),
                  "map solid under the spawn ladder - clear the authored "
                  "map around the spawn columns");

    GameT(LcdT& d) : disp(d) {}

    /** \brief The boot: driver lifecycle, then the one map paint
     *  (its own drawing cycle). */
    void init();

    /** \brief The frame: spawn, sim phases, the drawing cycle under
     *  the CS window, the closing lull and tile passes. One call =
     *  one frame; the pace belongs to the platform. */
    void loop();
private:
    void spawn();

    /* The one driver object of the platform: lifecycle calls ride
     * this reference; the blit path needs no object at all (the
     * driver's print path is static, no per-object state). */
    LcdT& disp;

    /* True if a live tank sits too close to the spawn point. */
    static bool is_booked(const point_t &pt, const Tank<LcdT> &tank);

    static void resolve_collision(Tank<LcdT> &t_moved, const Tank<LcdT> &t_stayed,
                                  MapT<LcdT> &map);

    void resolve_collision(Shell<LcdT> &shell, Tank<LcdT> &tank);

    /* The tile bookkeeping of one entity: the sprite rect plus the
     * rect of the last frame's sprite - the trail band print()/clear()
     * wipes lives inside the two, and the tiles under them are booked
     * for the frame's repaint pass (the flying shells erase the water
     * they cross). */
    void tile_book(const Entity<LcdT> &entity);
};

template <Disp LcdT>
void GameT<LcdT>::init()
{
  /* Lifecycle + the one map paint: the field geometry, the spawn
   * ladder and the map grid are compile-time constants of the
   * instantiation now, there is nothing left to compute here - the
   * map blits its flash grid once. */
  disp.init();

  /* The map paints in its own drawing cycle (on a shared bus
   * anything printed before the pick is ignored - CS high). */
  disp.select();
  map.draw();
  disp.unselect();
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
  {
    /* The per-side live count is rebuilt once per frame straight
     * from the Lobby state: the side lives on the entity as a tag,
     * and the corpses of the previous frame were culled by the
     * lull pass at its end - every active slot is a live tank
     * here. The spawn/kill counter bookkeeping is gone; kills
     * book nothing anywhere. The pass-level pin covers this scan
     * only: each later pass reads the header fresh to see the
     * earlier passes' spawns. */
    const int n_before = tanks.active_n();

    int n_side = 0;
    for (int t_i = 0; t_i < n_before; ++t_i)
      if (tanks[t_i]->side == s_i)
        ++n_side;

    /* Linear scan of the side's slots: spawn into the first free
     * ones until the side's quota is met. Booked slots are retried
     * on the following frames. The count is carried locally and
     * bumped on each issued tank. */
    for (int s = 0; s < cfg::spawn_slots && n_side < cfg::units_per_side; ++s)
    {
      const point_t pt = spawn_coords[s_i][s];

      /* One header read per slot, not per scanned tank: is_booked()
       * is an outlined call at -Os, the compiler reloaded the lobby
       * header on every scan tick (int-vs-int TBAA over runtime GEP
       * args, checked in the dump). The hoist sits at the slot level
       * on purpose - the same-pass spawns of the earlier slots ARE
       * booked into this slot's scan, a function-wide pin would
       * freeze a stale count. */
      const int n_live = tanks.active_n();

      bool booked = false;

      for (int t_i = 0; t_i < n_live; ++t_i)
      {
        if (!is_booked(pt, *tanks[t_i]))
          continue;

        booked = true;
        break;
      }
      if (booked)
        continue;

      /* Pool is quota-sized (2 x units_per_side) and active_n is
       * capped by it at every spawn (deaths are culled in the same
       * frame loop()), so sleep() hands a slot unconditionally. */
      Tank<LcdT>* free_tank = tanks.sleep();

      free_tank->respawn(pt.x, pt.y, s_i);
      tanks.activate();

      ++n_side;
    }
  }
}

template <Disp LcdT>
void GameT<LcdT>::resolve_collision(Tank<LcdT> &t_moved, const Tank<LcdT> &t_stayed, MapT<LcdT>& map)
{
  /* The house collision principle: a step does not pass onto an
   * occupied cell. Only the tank that actually displaced this frame
   * (action MOVE) reverts its step - a wall-pressed tank (BLOCK)
   * stays where it is, nothing displaces under a BLOCK print, no
   * ghost trails. The revert is the exact undo of the step (net
   * zero), the stuck pump feeds on it and turns the tank away. */
  if (t_moved.action != MOVE)
    return;

  const int dist_x = (t_moved.w + t_stayed.w) / 2;
  const int dist_y = (t_moved.h + t_stayed.h) / 2;

  if ((abs(t_moved.x - t_stayed.x) >= dist_x) || (abs(t_moved.y - t_stayed.y) >= dist_y))
    return;
  
  t_moved.back(true, map);
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

  /* Corpses take no book: the guard doubles as the kill() gate.
   * kill() clears and answers true exactly at the zero crossing
   * (armored hits keep life > 0 and kill() answers false), and a
   * re-fire on the same corpse is pre-blocked at the top. The
   * spawn quota is rebuilt from the Lobby side tags at each
   * spawn() - kills book nothing. */
  if (tank.life <= 0)
    return;

  if (abs(shell.dir - tank.dir) == 2)
    tank.life -= 1;
  else
    tank.life -= 2;

  /* A kill wipes the corpse rect (kill() clears) - the wipe can cut
   * into the tile layer, so it goes to the tile book of the frame. */
  if (tank.kill())
    tile_book(tank);
}

template <Disp LcdT>
void GameT<LcdT>::tile_book(const Entity<LcdT> &entity)
{
  const int wr = entity.w_img / 2;
  const int hr = entity.h_img / 2;

  /* The sprite rect of this frame. */
  map.tile_touch(entity.x - wr, entity.y - hr, entity.w_img, entity.h_img);

  /* The rect of the last frame's sprite: the entity stepped
   * dir * speed this frame. */
  int dx = 0;
  int dy = 0;

  switch (entity.dir)
  {
    case UP:    dy = -entity.speed; break;
    case DOWN:  dy =  entity.speed; break;
    case LEFT:  dx = -entity.speed; break;
    case RIGHT: dx =  entity.speed; break;
    default:    break;
  }

  map.tile_touch(entity.x - wr - dx, entity.y - hr - dy,
                 entity.w_img, entity.h_img);
}

template <Disp LcdT>
void GameT<LcdT>::loop()
{
  spawn();

  /* The tank lobby header is read once for the whole sim phase: no
   * pool state mutates between here and the closing lull pass -
   * kill() only drains lives, corpses stay active, and the pool
   * issued into during this phase is the shells one. At -Os
   * update()/resolve_collision()/isShot() stay outlined, and an
   * outlined call is an open write set for the compiler (the args
   * are runtime GEPs into this same object, int-vs-int TBAA doesn't
   * separate the lobby header from an int member) - it reloaded
   * active_n on every tick of every loop below. This pin turns it
   * into a callee-saved register across all of them. The frame's
   * four pin reads live at their own points of freshness (pool
   * counts shift between the points): n_tanks here, n_shells after
   * the tank phase (its shots grew the shells pool), the two draw
   * reads after the lull passes. */
  const int n_tanks = tanks.active_n();

  for (int t_i = 0; t_i < n_tanks; ++t_i)
  {
    tanks[t_i]->update(fld_x_end, fld_y_end, map);

    for (int t_c = 0; t_c < n_tanks; ++t_c)
    {
      /* The (i, i) pair is poisoned without this check: the tank
       * overlaps itself at zero distance and would push itself
       * (resolve() is asymmetric) every frame. */
      if (t_c == t_i) continue;

      resolve_collision(*tanks[t_i], *tanks[t_c], map);
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

  /* Fresh read: the shells issued in the tank phase above are
   * already counted in active_n here. The pool is stable through
   * this phase (explode() only drops life flags), so the outer
   * bound is read once instead of per tick as well. */
  const int n_shells = shells.active_n();

  for (int s_i = 0; s_i < n_shells; ++s_i)
  {
    if (!(shells[s_i]->life)) continue;

    shells[s_i]->update(fld_x_end, fld_y_end, map);

    /* The tank catches everything it overlaps, even the frame it
     * went down in - shells explode on the fresh corpse. Kills book
     * nothing anywhere (the spawn quota is rebuilt from the Lobby
     * side tags each frame); the life guard inside
     * resolve_collision() exists only for the kill() contract, an
     * extra hit on a corpse neither re-clears nor desyncs anything. */
    for (int t_i = 0; t_i < n_tanks; ++t_i)
      resolve_collision(*shells[s_i], *tanks[t_i]);

    /* A shell that went down this frame wiped its rect (update()
     * clears on death) - booked for the tile pass. */
    if (!(shells[s_i]->life))
      tile_book(*shells[s_i]);
  }

  /* The bricks blown away in the shell pass above repaint before
   * anything draws over their cells (the bits were marked from the
   * sim phase, inside this same drawing cycle). */
  map.flush_destroyed();

  for (const Shell<LcdT>* shell = shells.start(); shells.is_nEnd(); shell = shells.next())
    if (!(shell->life)) shells.lull();


  for (const Tank<LcdT>* tank = tanks.start(); tanks.is_nEnd(); tank = tanks.next())
    if (tank->life < 1) tanks.lull();

  /* Fresh reads for the draw phase: both lull passes above shrank
   * the pools, and print() mutates nothing, so each count is stable
   * for its whole pass - both pins pair up next to each other. */
  const int n_draw_tanks = tanks.active_n();
  const int n_draw_shells = shells.active_n();

  for (int t_i = 0; t_i < n_draw_tanks; ++t_i)
  {
    tanks[t_i]->print();

    /* The tank print covers and wipes tile cells - booked. */
    tile_book(*tanks[t_i]);
  }

  for (int s_i = 0; s_i < n_draw_shells; ++s_i)
  {
    shells[s_i]->print();

    /* Same for the shell print: its trail strip and sprite. */
    tile_book(*shells[s_i]);
  }

  /* The tile mask repaints last: everything the frame drew or wiped
   * over a tile cell is covered back - the classic drive-under
   * hiding, and the repair of the tiles the flying shells erase. */
  map.tile_flush();

  disp.unselect();
}
