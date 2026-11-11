/* Unit tests of the tank (Tank<LcdT>) - derived from the tank
 * contract in Game_Core/Game/README.md (section "Танк"): the one-px
 * step, the edge clamp, the wall step revert (net zero, no ghosts),
 * the push-back eaten by the wall, the rotate wrap, the respawn
 * reset, the isShot consume semantics, the kill wipe and the
 * deterministic AI ticks.
 *
 * The AI probabilities bind to the libc rand() stream - the seeds
 * are part of the contract; revisit them when пункт V (xorshift)
 * lands. The stuck counters/cooldowns are private: tested
 * behaviorally through the public fields and the fake panel log.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tank.hpp"

#include "fake_lcd.hpp"

#include <tuple>
#include <vector>

namespace {

using Lcd240 = FakeLcd<240, 240>; /* tank tests run on the F401 form */

using Tank240 = Tank<Lcd240>;
using Map240 = MapT<Lcd240>;

/* The first spot (left-to-right, top-down) where the hull is free
 * now AND after the one-px step right. */
bool find_free_pair(int& x, int& y)
{
    for (int yy = 6; yy <= 233; yy += 4)
        for (int xx = 6; xx <= 232; xx += 4)
            if (Map240::spawn_free(xx, yy) && Map240::spawn_free(xx + 1, yy))
            {
                x = xx;
                y = yy;
                return true;
            }
    return false;
}

/* A free spot right above a steel cell (row >= 3): the next step
 * down lands on the steel. */
bool find_spot_above_steel(int& x, int& y)
{
    for (int r = 3; r < Map240::rows; ++r)
        for (int c = 1; c < Map240::cols - 1; ++c)
            if (Map240::at(r, c) == MAP_STEEL)
            {
                const int xx = c * 8 + 4;
                const int yy = r * 8 - 7;
                if (Map240::spawn_free(xx, yy))
                {
                    x = xx;
                    y = yy;
                    return true;
                }
            }
    return false;
}

/* A free spot right below a steel cell: the back-step (up) lands on
 * the steel. */
bool find_spot_below_steel(int& x, int& y)
{
    for (int r = 1; r < Map240::rows; ++r)
        for (int c = 1; c < Map240::cols - 1; ++c)
            if (Map240::at(r, c) == MAP_STEEL)
            {
                const int xx = c * 8 + 4;
                const int yy = r * 8 + 14;
                if (yy <= 233 && Map240::spawn_free(xx, yy))
                {
                    x = xx;
                    y = yy;
                    return true;
                }
            }
    return false;
}

} /* namespace */

/* --- movement: the step, the edge, the wall --- */

TEST_CASE("tank_moves_one_px")
{
    Map240 map;
    int x, y;
    REQUIRE(find_free_pair(x, y));

    Tank240 t(x, y, RIGHT, 0);
    t.move(239, 239, map);

    CHECK(t.x == x + 1); /* the speed = 1 px step */
    CHECK(t.y == y);
    CHECK(t.action == MOVE);
}

TEST_CASE("tank_edge_clamps")
{
    Map240 map;
    Tank240 t(6, 30, LEFT, 0);

    t.move(239, 239, map); /* 6 - 1 < l_hlf(6): clamped back */
    CHECK(t.x == 6);
    CHECK(t.action == BLOCK);

    t.move(239, 239, map); /* stable: no drift along the edge */
    CHECK(t.x == 6);
    CHECK(t.action == BLOCK);
}

TEST_CASE("tank_wall_step_reverts")
{
    Map240 map;
    int x, y;
    REQUIRE(find_spot_above_steel(x, y));

    Tank240 t(x, y, DOWN, 0);
    for (int i = 0; i < 3; ++i)
    {
        t.move(239, 239, map); /* the step onto steel reverts: net zero */
        CHECK(t.x == x);
        CHECK(t.y == y);
        CHECK(t.action == BLOCK);
    }
}

/* --- the push-back --- */

TEST_CASE("tank_back_wall_eats_push")
{
    Map240 map;
    int x, y;
    REQUIRE(find_spot_below_steel(x, y));

    Tank240 t(x, y, DOWN, 0);
    t.back(true, map); /* the push up lands on steel: wall eats it */

    CHECK(t.x == x);
    CHECK(t.y == y);
    CHECK(t.action == BLOCK);
}

TEST_CASE("tank_back_blocks")
{
    Map240 map;
    int x, y;
    REQUIRE(find_free_pair(x, y));

    Tank240 t(x, y, RIGHT, 0);
    t.back(true, map); /* stuck=true books the BLOCK action */
    CHECK(t.action == BLOCK);
}

/* --- rotate / respawn --- */

TEST_CASE("tank_rotate_wraps")
{
    Tank240 t(100, 100, 0, 0);
    t.rotate(7);
    CHECK(t.dir == 3); /* 7 % 4 */
    t.rotate(4);
    CHECK(t.dir == 0); /* 4 % 4 */
}

TEST_CASE("tank_respawn_resets")
{
    Map240 map;
    int x, y;
    REQUIRE(find_free_pair(x, y));

    Tank240 t(x, y, 0, 0);
    t.life = 1;              /* spent some life */
    t.shot_st = true;
    (void)t.isShot(239, 239); /* a shot spent */

    t.respawn(30, 30, 0);
    CHECK(t.life == 2);
    CHECK(t.dir == 1);       /* side 0 -> dir 1 */

    /* the fire cooldown restarted: the first 59 ticks cannot fire */
    for (int i = 0; i < 59; ++i)
    {
        t.update(239, 239, map);
        CHECK_FALSE(t.isShot(239, 239));
    }
}

/* --- isShot: consume, aim, edge rejection --- */

TEST_CASE("tank_isshot_consumes")
{
    Tank240 t(100, 100, UP, 0);

    CHECK_FALSE(t.isShot(239, 239)); /* nothing loaded */

    t.shot_st = true;
    CHECK(t.isShot(239, 239)); /* the muzzle shot */

    Shell<Lcd240> s;
    t.shot(s); /* aims by the last isShot */
    CHECK(s.x == 100);
    CHECK(s.y == 100 - 6); /* the hull edge (h / 2 = 6) */
    CHECK(s.life);

    CHECK_FALSE(t.isShot(239, 239)); /* consumed: one poll = one shot */
}

TEST_CASE("tank_isshot_rejects_edge")
{
    Tank240 t(6, 30, LEFT, 0);
    t.shot_st = true;

    /* muzzle at 6 - 6 = 0: outside the field -> rejected (and the
     * flag is consumed anyway) */
    CHECK_FALSE(t.isShot(239, 239));
    CHECK_FALSE(t.isShot(239, 239));
}

/* --- kill: the wipe --- */

TEST_CASE("tank_kill_clears")
{
    FakeLcd<240, 240>::reset();
    Tank240 t(100, 100, 0, 0);

    CHECK_FALSE(t.kill()); /* life = 2: alive */

    t.life = 0;
    FakeLcd<240, 240>::reset();
    CHECK(t.kill()); /* dead: report + wipe */
    CHECK(FakeLcd<240, 240>::log.size() > 0); /* the clear() blit */
}

/* --- AI ticks: determinism and the fire cooldown --- */

TEST_CASE("tank_trajectory_deterministic")
{
    Map240 map;
    auto run = [&map]()
    {
        std::srand(7);
        Tank240 t(30, 30, 1, 0);
        std::vector<std::tuple<int, int, int>> trace;
        for (int i = 0; i < 200; ++i)
        {
            t.update(239, 239, map);
            trace.push_back({t.x, t.y, t.dir});
        }
        return trace;
    };

    const auto a = run();
    const auto b = run();

    REQUIRE(a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i)
        CHECK(a[i] == b[i]);
}

TEST_CASE("tank_fire_after_cooldown")
{
    Map240 map;
    std::srand(11);

    Tank240 t(30, 30, 1, 0);
    int shots = 0;
    for (int i = 0; i < 400; ++i)
    {
        t.update(239, 239, map);
        if (t.isShot(239, 239))
            ++shots;
    }
    CHECK(shots >= 1); /* the rolls fire within 400 ticks (seed 11) */
}
