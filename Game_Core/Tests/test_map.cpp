/* Unit tests of the terrain map (MapT<LcdT>) - derived from the map
 * contract in Game_Core/Game/README.md (section "Карта"): geometry
 * fold, legend lookup, hull passability, terrain collision, shell
 * hits, repaints and the tile mask. Both panel forms are covered -
 * the 240x240 exact fit (F401 form) and the 160x80 center-band crop
 * (H743 form). The recording fake panel (fake_lcd.hpp) observes the
 * blits; reset() it wherever the log is read.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "map.hpp"

#include "fake_lcd.hpp"

namespace {

using Lcd240 = FakeLcd<240, 240>; /* the F401 form - the whole map */
using Lcd160 = FakeLcd<160, 80>;  /* the H743 form - the center crop */

constexpr int CELL = MapT<Lcd240>::cell; /* the map cell, px */

/* First cell of the given kind on the 240x240 form (panel == the
 * authored map there): the known-answer scan for per-cell tables. */
bool first_cell_of(int& r, int& c, map_cell_t kind)
{
    using Map = MapT<Lcd240>;
    for (int rr = 0; rr < Map::rows; ++rr)
        for (int cc = 0; cc < Map::cols; ++cc)
            if (Map::at(rr, cc) == kind)
            {
                r = rr;
                c = cc;
                return true;
            }
    return false;
}

} /* namespace */

/* --- geometry and lookup (contract: rows/cols/row0/col0/at) --- */

TEST_CASE("map_full_fit_geometry")
{
    using Map = MapT<Lcd240>;
    CHECK(Map::rows == 30);
    CHECK(Map::cols == 30);
    CHECK(Map::row0 == 0); /* exact fit: the whole map, no crop */
    CHECK(Map::col0 == 0);
}

TEST_CASE("map_crop_center_band")
{
    using Map = MapT<Lcd160>;
    CHECK(Map::rows == 10); /* 80 / 8 */
    CHECK(Map::cols == 20); /* 160 / 8 */
    CHECK(Map::row0 == 10); /* (30 - 10) / 2 - the centered band */
    CHECK(Map::col0 == 5);  /* (30 - 20) / 2 */
}

TEST_CASE("map_at_wraps")
{
    using Map = MapT<Lcd240>; /* row0 == col0 == 0: panel == authored */
    CHECK(Map::at(-1, 0) == Map::at(Map::rows - 1, 0));
    CHECK(Map::at(Map::rows, 0) == Map::at(0, 0));
    CHECK(Map::at(0, -1) == Map::at(0, Map::cols - 1));
    CHECK(Map::at(0, Map::cols) == Map::at(0, 0));
}

TEST_CASE("map_legend_samples")
{
    using Map = MapT<Lcd240>;
    CHECK(Map::at(0, 0) == MAP_EMPTY);
    CHECK(Map::at(1, 1) == MAP_STEEL);
    CHECK(Map::at(14, 14) == MAP_WATER);
    CHECK(Map::at(2, 6) == MAP_BUSH);
}

TEST_CASE("map_grid_all_valid")
{
    auto valid = [](map_cell_t k)
    { return k == MAP_EMPTY || k == MAP_BRICK || k == MAP_STEEL
          || k == MAP_WATER || k == MAP_BUSH; };

    for (int r = 0; r < MapT<Lcd240>::rows; ++r)
        for (int c = 0; c < MapT<Lcd240>::cols; ++c)
            CHECK(valid(MapT<Lcd240>::at(r, c)));

    for (int r = 0; r < MapT<Lcd160>::rows; ++r)
        for (int c = 0; c < MapT<Lcd160>::cols; ++c)
            CHECK(valid(MapT<Lcd160>::at(r, c)));
}

TEST_CASE("map_crop_matches_source")
{
    using Map = MapT<Lcd160>;
    using Src = MapT<Lcd240>;
    for (int r = 0; r < Map::rows; ++r)
        for (int c = 0; c < Map::cols; ++c)
        {
            INFO("cell " << r << "," << c);
            CHECK(Map::at(r, c) == Src::at(Map::row0 + r, Map::col0 + c));
        }
}

/* --- spawn_free: the hull sits on passable cells --- */

TEST_CASE("spawn_free_blocked_by_solid")
{
    using Map = MapT<Lcd240>;
    int r, c;

    REQUIRE(first_cell_of(r, c, MAP_STEEL));
    CHECK_FALSE(Map::spawn_free(c * CELL + 4, r * CELL + 4));

    REQUIRE(first_cell_of(r, c, MAP_WATER));
    CHECK_FALSE(Map::spawn_free(c * CELL + 4, r * CELL + 4));

    REQUIRE(first_cell_of(r, c, MAP_BRICK));
    CHECK_FALSE(Map::spawn_free(c * CELL + 4, r * CELL + 4));
}

TEST_CASE("spawn_free_ladder_slots")
{
    using Map = MapT<Lcd240>;
    /* the ladder: spawn_slots spread evenly over the field height,
     * columns spawn_margin_px from the edges (config.hpp contract) */
    for (int i = 0; i < cfg::spawn_slots; ++i)
    {
        const int y = cfg::spawn_margin_px
            + i * (240 - 2 * cfg::spawn_margin_px) / (cfg::spawn_slots - 1);
        CHECK(Map::spawn_free(cfg::spawn_margin_px, y));
        CHECK(Map::spawn_free(240 - cfg::spawn_margin_px, y));
    }
}

/* --- blocks: the terrain table + edge clamping --- */

TEST_CASE("blocks_by_terrain")
{
    MapT<Lcd240> map;
    int r, c;

    REQUIRE(first_cell_of(r, c, MAP_STEEL));
    CHECK(map.blocks(c * CELL, r * CELL, CELL, CELL));

    REQUIRE(first_cell_of(r, c, MAP_WATER));
    CHECK(map.blocks(c * CELL, r * CELL, CELL, CELL));

    REQUIRE(first_cell_of(r, c, MAP_BRICK));
    CHECK(map.blocks(c * CELL, r * CELL, CELL, CELL));

    /* the whole brick blocks, the downed one does not */
    CHECK_FALSE(map.brick_down(r, c));
    (void)map.hit(c * CELL, r * CELL, CELL, CELL);
    CHECK(map.brick_down(r, c));
    CHECK_FALSE(map.blocks(c * CELL, r * CELL, CELL, CELL));

    REQUIRE(first_cell_of(r, c, MAP_EMPTY));
    CHECK_FALSE(map.blocks(c * CELL, r * CELL, CELL, CELL));

    REQUIRE(first_cell_of(r, c, MAP_BUSH));
    CHECK_FALSE(map.blocks(c * CELL, r * CELL, CELL, CELL));
}

TEST_CASE("blocks_clamps_out_of_field")
{
    MapT<Lcd240> map;
    /* a rect partly outside the field: clamped, no UB, the in-field
     * part rules - the (0,0) corner is empty (known-answer) */
    CHECK_FALSE(map.blocks(-4, -4, 8, 8));
}

/* --- hit: shell outcome per terrain kind --- */

TEST_CASE("hit_steel_kills_only")
{
    MapT<Lcd240> map;
    int r, c;
    REQUIRE(first_cell_of(r, c, MAP_STEEL));

    CHECK(map.hit(c * CELL, r * CELL, CELL, CELL)); /* the shell dies */

    /* the steel itself is untouched: no brick anywhere went down */
    int rb, cb;
    REQUIRE(first_cell_of(rb, cb, MAP_BRICK));
    CHECK_FALSE(map.brick_down(rb, cb));
}

TEST_CASE("hit_brick_destroys")
{
    MapT<Lcd240> map;
    int r, c;
    REQUIRE(first_cell_of(r, c, MAP_BRICK));

    CHECK_FALSE(map.brick_down(r, c));
    CHECK(map.hit(c * CELL, r * CELL, CELL, CELL));
    CHECK(map.brick_down(r, c));
}

TEST_CASE("hit_downed_brick_passes")
{
    MapT<Lcd240> map;
    int r, c;
    REQUIRE(first_cell_of(r, c, MAP_BRICK));

    (void)map.hit(c * CELL, r * CELL, CELL, CELL);
    /* the downed brick is emptiness for the next shell - the mutant
     * target is the old "dead on any brick cell" behavior */
    CHECK_FALSE(map.hit(c * CELL, r * CELL, CELL, CELL));
}

TEST_CASE("hit_water_and_empty_pass")
{
    MapT<Lcd240> map;
    int r, c;

    REQUIRE(first_cell_of(r, c, MAP_WATER));
    CHECK_FALSE(map.hit(c * CELL, r * CELL, CELL, CELL));

    REQUIRE(first_cell_of(r, c, MAP_EMPTY));
    CHECK_FALSE(map.hit(c * CELL, r * CELL, CELL, CELL));
}

/* --- flush_destroyed: one dirty cell - one repaint, then silence --- */

TEST_CASE("flush_repaints_only_dirty")
{
    FakeLcd<240, 240>::reset();
    MapT<Lcd240> map;
    int r, c;
    REQUIRE(first_cell_of(r, c, MAP_BRICK));

    map.flush_destroyed();
    CHECK(FakeLcd<240, 240>::log.size() == 0); /* nothing destroyed yet */

    (void)map.hit(c * CELL, r * CELL, CELL, CELL);
    map.flush_destroyed();
    REQUIRE(FakeLcd<240, 240>::log.size() == 1);
    CHECK(FakeLcd<240, 240>::log[0].x == c * CELL);
    CHECK(FakeLcd<240, 240>::log[0].y == r * CELL);
    CHECK(FakeLcd<240, 240>::log[0].w == CELL);
    CHECK(FakeLcd<240, 240>::log[0].h == CELL);

    FakeLcd<240, 240>::reset(); /* the log accumulates - clear it */
    map.flush_destroyed();
    CHECK(FakeLcd<240, 240>::log.size() == 0); /* the dirty bit consumed */
}

/* --- tile mask: book, dedup, cap, flush --- */

TEST_CASE("tile_mask_books_and_flushes")
{
    FakeLcd<240, 240>::reset();
    MapT<Lcd240> map;
    int r, c;

    REQUIRE(first_cell_of(r, c, MAP_STEEL));
    map.tile_touch(c * CELL, r * CELL, CELL, CELL);
    REQUIRE(first_cell_of(r, c, MAP_WATER));
    map.tile_touch(c * CELL, r * CELL, CELL, CELL);
    REQUIRE(first_cell_of(r, c, MAP_BUSH));
    map.tile_touch(c * CELL, r * CELL, CELL, CELL);
    REQUIRE(first_cell_of(r, c, MAP_BRICK));
    map.tile_touch(c * CELL, r * CELL, CELL, CELL);
    REQUIRE(first_cell_of(r, c, MAP_EMPTY));
    map.tile_touch(c * CELL, r * CELL, CELL, CELL); /* not booked */

    map.tile_flush();
    CHECK(FakeLcd<240, 240>::log.size() == 4);

    /* the downed brick is not booked either */
    FakeLcd<240, 240>::reset(); /* the log accumulates - clear it */
    REQUIRE(first_cell_of(r, c, MAP_BRICK));
    (void)map.hit(c * CELL, r * CELL, CELL, CELL);
    map.tile_touch(c * CELL, r * CELL, CELL, CELL);
    map.tile_flush();
    CHECK(FakeLcd<240, 240>::log.size() == 0);
}

TEST_CASE("tile_mask_dedup_cap")
{
    FakeLcd<240, 240>::reset();
    MapT<Lcd240> map;

    int r, c;
    REQUIRE(first_cell_of(r, c, MAP_STEEL));
    map.tile_touch(c * CELL, r * CELL, CELL, CELL);
    map.tile_touch(c * CELL, r * CELL, CELL, CELL); /* dedup */
    map.tile_touch(0, 0, 240, 240);                 /* floods the book */

    map.tile_flush();
    CHECK(FakeLcd<240, 240>::log.size() == 32);     /* the cap */

    FakeLcd<240, 240>::reset(); /* the log accumulates - clear it */
    map.tile_flush();
    CHECK(FakeLcd<240, 240>::log.size() == 0);      /* the book emptied */
}

/* --- draw: the window, not a pixel outside it --- */

TEST_CASE("draw_covers_window")
{
    using Map = MapT<Lcd160>;
    FakeLcd<160, 80>::reset();
    Map map;

    map.draw();

    int non_empty = 0;
    for (int r = 0; r < Map::rows; ++r)
        for (int c = 0; c < Map::cols; ++c)
            if (Map::at(r, c) != MAP_EMPTY)
                ++non_empty;

    REQUIRE(FakeLcd<160, 80>::log.size() == non_empty);
    for (const auto& b : FakeLcd<160, 80>::log)
    {
        CHECK(b.w == CELL);
        CHECK(b.h == CELL);
        CHECK(b.x + b.w <= 160); /* the BLOCK contract: inside the panel */
        CHECK(b.y + b.h <= 80);
    }
}
