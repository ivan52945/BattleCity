#pragma once

/* Gameplay knobs. Field geometry is NOT here any more: the panel size
 * comes at runtime (Disp_Iface), the spawn layout is computed from it
 * in Game::init() (two columns near the field edges, slots spread
 * evenly over the height). */
namespace cfg {

/* Distance of the tank spawn columns from the field edges, px. */
constexpr int spawn_margin_px = 30;

/* Spawn slots per side, spread evenly over the field height.
 * 7 slots x 30 px fills a classic 240px panel: y = 30..210 -
 * the same ladder the old literal comment table described. */
constexpr int spawn_slots = 7;

/* Maximum simultaneous tanks per side. Upper bound for the side's
 * spawn slots count; the pool size (Lobby) must hold both sides. */
constexpr int units_per_side = 4;

}
