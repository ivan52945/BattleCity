#pragma once

/** @brief Gameplay knobs.
 *
 *  Field geometry is NOT here: the panel size is a compile-time
 *  constant of the driver instantiation (static constexpr w()/h()),
 *  the field edges and the spawn ladder fold at compile time in GameT.
 */
namespace cfg {

/** Distance of the tank spawn columns from the field edges, px. */
constexpr int spawn_margin_px = 30;

/** Spawn slots per side, spread evenly over the field height.
 *  7 slots x 30 px fills a classic 240px panel: y = 30..210 - the
 *  same ladder the old literal comment table described. */
constexpr int spawn_slots = 7;

/** Maximum simultaneous tanks per side. Upper bound for the side's
 *  spawn slots count; the pool size (Lobby) must hold both sides -
 *  and is derived from this knob in GameT, keep it the single source. */
constexpr int units_per_side = 4;

static_assert(units_per_side <= spawn_slots,
              "quota above the spawn ladder: slots would saturate and "
              "the booked-scan would run every frame");
static_assert(units_per_side > 0, "quota must be positive");

/** Map grid cell, px. The terrain sprites are authored at this size
 *  (MAP_SPR in assets.hpp; map.hpp static_asserts the match) and both
 *  platform panels divide by it exactly (240/160/80). */
constexpr int map_cell_px = 8;

}
