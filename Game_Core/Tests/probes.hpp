/* Shared known-answer probes over the authored map (the F401 form:
 * panel == the authored grid) for the test suites - terrain scans
 * that locate per-cell table fixtures and terrain-clean flight
 * paths. The probes read only the contract surface (at() and the
 * cell kinds), never the implementation internals. */

#pragma once

#include "map.hpp"

#include "fake_lcd.hpp"

namespace probes {

using Lcd240 = FakeLcd<240, 240>; /* the F401 form - the whole map */
using Map240 = MapT<Lcd240>;

/* First cell of the given kind on the 240x240 form (known-answer
 * scan for the per-cell decision tables). */
inline bool first_cell_of(int& r, int& c, map_cell_t kind)
{
    for (int rr = 0; rr < Map240::rows; ++rr)
        for (int cc = 0; cc < Map240::cols; ++cc)
            if (Map240::at(rr, cc) == kind)
            {
                r = rr;
                c = cc;
                return true;
            }
    return false;
}

/* A column x whose cells over [y0 .. y0+rows) are all passable
 * (empty or bush) - for the ALIVE assertions the flight path must be
 * terrain-clean. */
inline bool column_clean(const int x, const int y0, const int rows)
{
    for (int r = y0 / Map240::cell; r <= (y0 + rows - 1) / Map240::cell; ++r)
    {
        const map_cell_t k = Map240::at(r, x / Map240::cell);
        if (k != MAP_EMPTY && k != MAP_BUSH)
            return false;
    }
    return true;
}

/* A clean y for a vertical (DOWN) flight at column x: the hull after
 * the speed-3 step (rows (y+2)/8 .. (y+4)/8) stays passable. */
inline bool find_clean_flight_y(const int x, int& y)
{
    for (int yy = 4; yy <= 233; ++yy)
        if (column_clean(x, yy + 2, 3))
        {
            y = yy;
            return true;
        }
    return false;
}

} /* namespace probes */
