/* Unit tests of the shell (Shell<LcdT>) - derived from the shell
 * contract (the doc comments in shell.hpp) and the layer invariants
 * in Game_Core/Game/README.md: the speed-3 flight step, the oriented
 * hull (1x3 vertical / 3x1 horizontal), the edge death zone (BVA),
 * the terrain outcomes (steel kills, water passes, brick kills +
 * destroys, a downed brick passes), the wipe on death and the
 * explode / get_out pair.
 *
 * The DEATH assertions hold over any terrain (the edge check runs
 * inside move() over the map whatever it holds); the ALIVE
 * assertions scan for a terrain-clean flight path first (probes.hpp).
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "shell.hpp"

#include "probes.hpp"

namespace {

using Shell240 = Shell<probes::Lcd240>;
using Map240 = probes::Map240;

constexpr int CELL = Map240::cell;

/* Arms the shell at the point and runs one flight tick: answers the
 * life after the step. */
bool one_tick_life(int x, int y, int dir)
{
    Map240 map;
    Shell240 s(x, y, dir, 0);
    s.update(239, 239, map);
    return s.life;
}

} /* namespace */

/* --- the flight step and the orientation --- */

TEST_CASE("shell_moves_three_px")
{
    Map240 map;
    int y;
    REQUIRE(probes::find_clean_flight_y(100, y));

    Shell240 s(100, y, DOWN, 0);
    s.move(239, 239, map);

    CHECK(s.y == y + 3); /* the speed = 3 px step */
    CHECK(s.x == 100);
    CHECK(s.life);       /* clean path: alive */
}

TEST_CASE("shell_orients_hull")
{
    Shell240 v(100, 100, UP, 0);
    CHECK(v.w_img == 1); /* the vertical flight is 1x3 */
    CHECK(v.h_img == 3);

    Shell240 h(100, 100, RIGHT, 0);
    CHECK(h.w_img == 3); /* the horizontal flight is 3x1 */
    CHECK(h.h_img == 1);
}

/* --- the edge death zone (the death asserts hold over any terrain) --- */

TEST_CASE("shell_edge_death_bva")
{
    /* the step lands outside the alive zone [1 .. 238]: dead */
    CHECK_FALSE(one_tick_life(0, 120, DOWN));   /* x < 1 */
    CHECK_FALSE(one_tick_life(239, 120, DOWN)); /* x > 238 */
    CHECK_FALSE(one_tick_life(100, 3, UP));     /* y - 3 = 0 < 1 */
    CHECK_FALSE(one_tick_life(100, 237, DOWN)); /* y - 3 = 240 > 238 */

    /* the boundary itself stays alive (terrain-clean flight) */
    int y;
    REQUIRE(probes::find_clean_flight_y(1, y));
    CHECK(one_tick_life(1, y, DOWN)); /* x = 1: the alive edge */
}

TEST_CASE("shell_edge_death_wipes")
{
    FakeLcd<240, 240>::reset();
    Map240 map;

    Shell240 s(100, 2, UP, 0);
    s.update(239, 239, map); /* 2 - 3 = -1: dead, update() wipes */

    CHECK_FALSE(s.life);
    CHECK(FakeLcd<240, 240>::log.size() == 1); /* the wipe blit */
}

/* --- terrain outcomes --- */

TEST_CASE("shell_steel_kills")
{
    Map240 map;
    int r, c;
    REQUIRE(probes::first_cell_of(r, c, MAP_STEEL));

    Shell240 s(c * CELL + 4, r * CELL - 2, DOWN, 0);
    s.update(239, 239, map); /* the step lands the hull on the steel */

    CHECK_FALSE(s.life);
    CHECK_FALSE(map.brick_down(r, c)); /* the steel itself is untouched */
}

TEST_CASE("shell_water_passes")
{
    Map240 map;
    int r, c;
    REQUIRE(probes::first_cell_of(r, c, MAP_WATER));

    Shell240 s(c * CELL + 4, r * CELL - 2, DOWN, 0);
    s.update(239, 239, map); /* the hull sits on the water cell */
    CHECK(s.life);

    s.update(239, 239, map); /* crosses it: still alive */
    CHECK(s.life);
}

TEST_CASE("shell_brick_destroys")
{
    Map240 map;
    int r, c;
    REQUIRE(probes::first_cell_of(r, c, MAP_BRICK));
    CHECK_FALSE(map.brick_down(r, c));

    Shell240 s(c * CELL + 4, r * CELL - 2, DOWN, 0);
    s.update(239, 239, map); /* the step blows the brick and dies */

    CHECK_FALSE(s.life);
    CHECK(map.brick_down(r, c));
}

TEST_CASE("shell_downed_brick_passes")
{
    Map240 map;
    int r, c;
    REQUIRE(probes::first_cell_of(r, c, MAP_BRICK));

    Shell240 s(c * CELL + 4, r * CELL - 2, DOWN, 0);
    s.update(239, 239, map); /* the first shell blows the brick */
    CHECK_FALSE(s.life);

    Shell240 s2(c * CELL + 4, r * CELL - 2, DOWN, 0);
    s2.update(239, 239, map); /* the downed cell is emptiness now */
    CHECK(s2.life);
}

/* --- explode / get_out: the kill and the re-arm --- */

TEST_CASE("shell_getout_rearms")
{
    Shell240 s(100, 100, UP, 0);

    s.explode();
    CHECK_FALSE(s.life);

    s.get_out(100, 94, UP, 0); /* re-armed at the point */
    CHECK(s.life);
    CHECK(s.x == 100);
    CHECK(s.y == 94);
}
