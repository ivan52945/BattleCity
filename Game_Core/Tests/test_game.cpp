/* Unit tests of the orchestrator (GameT<LcdT>) - derived from the
 * frame order contract (Game/README.md + the frame order section of
 * Game/AGENTS.md): init paints only, the first loop spawns the full
 * quota, every frame's blits live inside one (select .. unselect)
 * CS window, the tile mask closes the frame, the destroyed brick
 * repaints before the draws, the corpses are culled and the quota
 * refills, and the shell resolves: an enemy shell kills, a friendly
 * shell explodes without damage.
 *
 * The shells are injected straight into the public pool to aim them;
 * the AI is pinned by the seed.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "game.hpp"

#include "probes.hpp"
#include "fake_lcd.hpp"

namespace {

using Lcd240 = probes::Lcd240;
using Game240 = GameT<Lcd240>;
using Map240 = probes::Map240;

constexpr int CELL = Map240::cell;

bool is_map_sprite(const uint16_t* img)
{
    return img == map_brick || img == map_steel
        || img == map_water || img == map_bush;
}

bool is_tank_frame(const uint16_t* img)
{
    const auto* ally = &tank_ally_frames[0][0][0];
    const auto* enemy = &tank_enemy_frames[0][0][0];
    return (img >= ally
            && img < ally + sizeof(tank_ally_frames) / sizeof(uint16_t))
        || (img >= enemy
            && img < enemy + sizeof(tank_enemy_frames) / sizeof(uint16_t));
}

} /* namespace */

/* --- init: paint only --- */

TEST_CASE("game_init_paints_only")
{
    FakeLcd<240, 240>::reset();
    Lcd240 lcd;
    Game240 game(lcd);

    game.init();

    int non_empty = 0;
    for (int r = 0; r < Map240::rows; ++r)
        for (int c = 0; c < Map240::cols; ++c)
            if (Map240::at(r, c) != MAP_EMPTY)
                ++non_empty;

    CHECK(game.tanks.active_n() == 0); /* the spawn belongs to loop() */

    REQUIRE(FakeLcd<240, 240>::log.size() == non_empty);
    REQUIRE(FakeLcd<240, 240>::events.size() == 3);
    CHECK(FakeLcd<240, 240>::events[0].first == 'i');
    CHECK(FakeLcd<240, 240>::events[1] == std::make_pair('s', size_t{0}));
    CHECK(FakeLcd<240, 240>::events[2]
          == std::make_pair('u', size_t{non_empty})); /* map paint in
                                                         its own window */
}

/* --- the first frame fills the quota --- */

TEST_CASE("game_first_loop_spawns_quota")
{
    Lcd240 lcd;
    Game240 game(lcd);
    game.init();

    std::srand(5);
    game.loop();

    CHECK(game.tanks.active_n() == 2 * cfg::units_per_side);
}

/* --- the CS window discipline --- */

TEST_CASE("game_blits_inside_cs_window")
{
    FakeLcd<240, 240>::reset();
    Lcd240 lcd;
    Game240 game(lcd);
    game.init();

    std::srand(5);
    for (int f = 0; f < 3; ++f)
        game.loop();

    const auto& ev = FakeLcd<240, 240>::events;
    const auto& log = FakeLcd<240, 240>::log;

    /* init cycle (i + its own (s, u) pair) + one (s, u) pair per
     * frame */
    REQUIRE(ev.size() == 3 + 2 * 3);
    CHECK(ev[0].first == 'i');

    /* no blits between frames: each select() fires at the very count
     * the previous unselect() left */
    for (size_t k = 3; k < ev.size(); k += 2)
        CHECK(ev[k].second == ev[k - 1].second);

    /* and every blit is inside some window */
    CHECK(ev.back().second == log.size());
}

/* --- the tile mask closes the frame --- */

TEST_CASE("game_tile_flush_closes_frame")
{
    FakeLcd<240, 240>::reset();
    Lcd240 lcd;
    Game240 game(lcd);
    game.init();

    std::srand(5);
    for (int f = 0; f < 40; ++f)
        game.loop();

    const auto& ev = FakeLcd<240, 240>::events;
    const auto& log = FakeLcd<240, 240>::log;

    bool any_mask = false;

    /* per frame window: once a terrain-sprite blit happens (the tile
     * mask repaint), nothing but terrain blits follow in that frame */
    for (size_t k = 1; k + 1 < ev.size(); k += 2)
    {
        bool seen_map = false;
        for (size_t i = ev[k].second; i < ev[k + 1].second; ++i)
        {
            if (is_map_sprite(log[i].img))
            {
                seen_map = true;
                any_mask = true;
            }
            else
                CHECK_FALSE(seen_map); /* a print after the mask */
        }
    }

    CHECK(any_mask); /* over 40 roaming frames the mask did fire */
}

/* --- the destroyed brick repaints before the draws --- */

TEST_CASE("game_destroyed_brick_repaints_before_draws")
{
    FakeLcd<240, 240>::reset();
    Lcd240 lcd;
    Game240 game(lcd);
    game.init();

    int r = -1, c = -1;
    for (int rr = 0; rr < Map240::rows && r < 0; ++rr)
        for (int cc = 6; cc <= 23 && r < 0; ++cc)
            if (Map240::at(rr, cc) == MAP_BRICK)
            {
                r = rr;
                c = cc;
            }
    REQUIRE(r >= 0); /* the map has a brick clear of the spawn columns */

    /* an enemy shell right above the brick, flying down */
    Shell<Lcd240>* s = game.shells.activate();
    REQUIRE(s != nullptr);
    s->get_out(c * CELL + 4, r * CELL - 2, DOWN, 1);

    std::srand(5);
    game.loop();

    const auto& log = FakeLcd<240, 240>::log;

    int clear_at_brick = -1;
    int first_tank = -1;
    for (size_t i = 0; i < log.size(); ++i)
    {
        if (clear_at_brick < 0 && log[i].img == img_clear
            && log[i].x == c * CELL && log[i].y == r * CELL)
            clear_at_brick = (int)i;
        if (first_tank < 0 && is_tank_frame(log[i].img))
            first_tank = (int)i;
    }

    REQUIRE(clear_at_brick >= 0);       /* the brick repaint happened */
    REQUIRE(first_tank >= 0);           /* the frame drew the tanks */
    CHECK(clear_at_brick < first_tank); /* repaint BEFORE any print */
    CHECK(game.map.brick_down(r, c));   /* and the brick is down */
}

/* --- corpses are culled, the quota refills --- */

TEST_CASE("game_corpse_culled_then_refilled")
{
    Lcd240 lcd;
    Game240 game(lcd);
    game.init();

    std::srand(5);
    game.loop();
    REQUIRE(game.tanks.active_n() == 2 * cfg::units_per_side);

    game.tanks[0]->life = 0; /* a corpse in the pool */

    game.loop(); /* the lull pass culls it */
    CHECK(game.tanks.active_n() == 2 * cfg::units_per_side - 1);

    game.loop(); /* the quota respawns the slot */
    CHECK(game.tanks.active_n() == 2 * cfg::units_per_side);
}

/* --- shell resolve: enemy kills, friendly explodes only --- */

TEST_CASE("game_shell_kills_enemy_tank")
{
    FakeLcd<240, 240>::reset();
    Lcd240 lcd;
    Game240 game(lcd);
    game.init();

    /* an enemy shell right above the ally spawn slot (30, 30) */
    Shell<Lcd240>* s = game.shells.activate();
    REQUIRE(s != nullptr);
    s->get_out(30, 22, DOWN, 1);

    std::srand(5);
    game.loop();

    CHECK(game.shells.active_n() == 0); /* shell exploded, culled */
    CHECK(game.tanks.active_n() == 2 * cfg::units_per_side - 1);
}

TEST_CASE("game_friendly_shell_no_damage")
{
    FakeLcd<240, 240>::reset();
    Lcd240 lcd;
    Game240 game(lcd);
    game.init();

    Shell<Lcd240>* s = game.shells.activate();
    REQUIRE(s != nullptr);
    s->get_out(30, 22, DOWN, 0); /* friendly fire over the same slot */

    std::srand(5);
    game.loop();

    CHECK(game.tanks.active_n() == 2 * cfg::units_per_side); /* nobody down */
    CHECK(game.tanks[0]->life == 2); /* the hit tank kept its life */
    CHECK(game.shells.active_n() == 0); /* the shell still culled */
}
