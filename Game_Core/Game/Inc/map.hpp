#pragma once

#include <array>
#include <cstdint>

#include <assets.hpp>
#include "config.hpp"
#include "disp.hpp"

/** \brief ===== The map: a char grid, one symbol per terrain kind =====
 *
 *  The field is authored as an array of strings (map_layout::rows_txt
 *  below) and a constexpr pass folds it into a flash-resident cell
 *  grid at build time - no runtime parsing, the map image is plain
 *  rodata like every other asset. The panel grid derives from the
 *  driver instantiation: cells of cfg::map_cell_px tile LcdT::w()/h()
 *  exactly, the panel shows a window of the authored grid (a larger
 *  panel tiles it).
 *
 *  The symbols (rows_wellformed()/symbols_valid() hold the discipline):
 *
 *    '.'  empty field
 *    '#'  brick - blocks tanks, shells blow it away (the destroyed
 *         cells pass traffic and repaint in the draw phase)
 *    'X'  steel - blocks tanks and shells, indestructible
 *    '~'  water - blocks tanks, shells fly over
 *    '*'  bush  - passable, painted over the sprites that touch it
 *         (the bush pass of the Game) - the classic mask
 *
 *  The authoring keep-out: the spawn hulls. Every instantiation
 *  static_asserts its spawn ladder free of solids through
 *  MapT::spawn_free() - a map edit that blocks a spawn is a build
 *  error of that platform, not a stuck tank on the field.
 */

/** \brief The terrain kinds of the authored grid (the legend above). */
enum map_cell_t : uint8_t
{
    MAP_EMPTY,
    MAP_BRICK,
    MAP_STEEL,
    MAP_WATER,
    MAP_BUSH,
};

namespace map_layout {

/* The authored grid: rows x cols symbols per the legend above. */
inline constexpr int rows = 30;
inline constexpr int cols = 30;

inline constexpr const char* rows_txt[rows] = {
    "..............................",
    ".X..........................X.",
    ".....***..............***.....",
    ".....***..............***.....",
    "..............................",
    ".....####............####.....",
    ".....####............####.....",
    ".....####............####.....",
    "........X............X........",
    ".....#..................#.....",
    ".....#***#..........#***#.....",
    ".....#***#..........#***#.....",
    "..........####..####..........",
    "............~~~~~~............",
    "............~~~~~~............",
    "............~~~~~~............",
    "............~~~~~~............",
    "..........####..####..........",
    ".....#***#..........#***#.....",
    ".....#***#..........#***#.....",
    ".....#..................#.....",
    "........X............X........",
    ".....####............####.....",
    ".....####............####.....",
    ".....####............####.....",
    "..............................",
    ".....***..............***.....",
    ".....***..............***.....",
    ".X..........................X.",
    "..............................",
};

/* The well-formedness discipline, both folding at compile time: a
 * typo in the authored map is a build error, not a runtime
 * surprise. */
constexpr bool rows_wellformed()
{
    for (int r = 0; r < rows; ++r)
    {
        int len = 0;
        while (rows_txt[r][len] != '\0')
            ++len;

        if (len != cols)
            return false;
    }

    return true;
}

constexpr bool symbols_valid()
{
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
        {
            switch (rows_txt[r][c])
            {
                case '.': case ' ':
                case '#': case 'X': case '~': case '*':
                    break;
                default:
                    return false;
            }
        }

    return true;
}

static_assert(rows_wellformed(), "a map row is not exactly cols symbols long");
static_assert(symbols_valid(),  "unknown map symbol - see the legend in map.hpp");

/* The flash grid: one byte per cell, folded from the strings. */
constexpr std::array<uint8_t, rows * cols> make_grid()
{
    std::array<uint8_t, rows * cols> grid = {};

    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
        {
            switch (rows_txt[r][c])
            {
                case '#': grid[r * cols + c] = MAP_BRICK; break;
                case 'X': grid[r * cols + c] = MAP_STEEL; break;
                case '~': grid[r * cols + c] = MAP_WATER; break;
                case '*': grid[r * cols + c] = MAP_BUSH;  break;
                default:  grid[r * cols + c] = MAP_EMPTY; break;
            }
        }

    return grid;
}

inline constexpr std::array<uint8_t, rows * cols> grid = make_grid();

} /* namespace map_layout */

/* The map of one Game, typed against the driver of that Game - the
 * same docking as the Game itself: the blit path dials the static
 * driver print, the panel geometry is a compile-time constant of the
 * instantiation. */
/** \brief The terrain grid over the panel: constexpr-compiled from
 *  the authored grid, plus the runtime destruction/tile state of the
 *  frame.
 *
 *  \tparam LcdT the driver type of the owning Game */
template <Disp LcdT>
class MapT
{
public:
    /* The cell: the cfg knob, authored-sprite sized. */
    static constexpr int cell = cfg::map_cell_px;

    static_assert(cfg::map_cell_px == MAP_SPR,
                  "map cell size must match the authored sprite size");

    /* The panel grid: cells tile the panel exactly (both platform
     * panels are multiples of the cell). */
    static constexpr int rows = LcdT::h() / cell;
    static constexpr int cols = LcdT::w() / cell;

    static_assert((LcdT::w() % cell == 0) && (LcdT::h() % cell == 0),
                  "panel size is not a multiple of the map cell");

    /* The source window into the authored grid: a smaller panel shows
     * the center band, a larger one tiles it (the wrap in at()). */
    static constexpr int row0 = (map_layout::rows - rows) / 2;
    static constexpr int col0 = (map_layout::cols - cols) / 2;

    /** \brief The cell kind under the panel cell (authored grid
     *  lookup, wrapped: the index runs modulo the authored dims,
     *  negatives corrected - the edges of the authored grid fold
     *  together).
     *
     *  \param row panel-cell row
     *  \param col panel-cell column
     *  \return the terrain kind of the cell */
    static constexpr map_cell_t at(const int row, const int col)
    {
        int sr = (row + row0) % map_layout::rows;
        int sc = (col + col0) % map_layout::cols;

        if (sr < 0) sr += map_layout::rows;
        if (sc < 0) sc += map_layout::cols;

        return (map_cell_t)map_layout::grid[sr * map_layout::cols + sc];
    }

    /** \brief The compile-time spawn check: the whole tank hull at
     *  the point must sit on passable cells (GameT static_asserts
     *  its whole spawn ladder through this). The ladder margins keep
     *  the hull inside the field - no clamp here.
     *
     *  \param x hull center x, px
     *  \param y hull center y, px
     *  \return true when no hull cell is solid (brick/steel/water) */
    static constexpr bool spawn_free(const int x, const int y)
    {
        constexpr int hw = TANK_W / 2;
        constexpr int hh = TANK_H / 2;

        for (int r = (y - hh) / cell; r <= (y + hh) / cell; ++r)
            for (int c = (x - hw) / cell; c <= (x + hw) / cell; ++c)
            {
                switch (at(r, c))
                {
                    case MAP_BRICK:
                    case MAP_STEEL:
                    case MAP_WATER:
                        return false;
                    default:
                        break;
                }
            }

        return true;
    }

    MapT() = default;

    /** \brief The destruction state of a brick cell (panel coords):
     *  the tile pass and the collision read the bits internally; this
     *  is the public query form (host checks, future win/lose logic).
     *
     *  \param r panel-cell row
     *  \param c panel-cell column
     *  \return true when the brick cell is destroyed */
    bool brick_down(const int r, const int c) const
    {
        return bit_get(destroyed, r * cols + c);
    }

    /** \brief The boot paint of the whole panel (init, inside the
     *  drawing cycle): one terrain-sprite blit per non-empty cell
     *  (the emptiness is the background). */
    void draw() const;

    /** \brief Tank movement query: does any solid cell overlap the
     *  rect? Steel/water block, a whole brick blocks, a downed brick,
     *  bush and emptiness do not. The rect is origin-based (x/y =
     *  the top-left corner, w/h = the extents), the callers shift
     *  their centered hulls; the out-of-field edges clamp in.
     *
     *  \param x rect left, px
     *  \param y rect top, px
     *  \param w rect width, px
     *  \param h rect height, px
     *  \return true when a solid cell overlaps the rect */
    bool blocks(const int x, const int y, const int w, const int h) const;

    /** \brief Shell impact on the rect: steel kills the shell, water
     *  lets it pass, the whole brick cells under the rect are blown
     *  away (destroyed + dirty - marked for the draw-phase repaint)
     *  and kill it; a downed brick, water and emptiness pass.
     *
     *  \param x rect left, px
     *  \param y rect top, px
     *  \param w rect width, px
     *  \param h rect height, px
     *  \return true = the shell dies */
    bool hit(const int x, const int y, const int w, const int h);

    /** \brief The destroyed-brick repaint: once per frame inside the
     *  drawing cycle (the bits are marked from the sim phase): every
     *  dirty cell blits one clear strip, the dirty bit consumes. */
    void flush_destroyed();

    /** \brief The tile mask, booking side: every tile cell under the
     *  rect (steel, water, bush, whole brick; a downed brick and
     *  emptiness skip) is booked for the frame's repaint pass,
     *  deduplicated; the book overflow (32) is dropped until the
     *  next frame.
     *
     *  \param x rect left, px
     *  \param y rect top, px
     *  \param w rect width, px
     *  \param h rect height, px */
    void tile_touch(const int x, const int y, const int w, const int h);

    /** \brief The tile mask, repaint side: one terrain-sprite blit
     *  per booked cell (a destroyed brick skips - its repaint went
     *  through flush_destroyed), then the book empties. Runs as the
     *  last blit pass of the frame: the drive-under-the-bush hiding
     *  and the repair of the tiles the flying shells erase. */
    void tile_flush();

private:
    static constexpr int cells = rows * cols;
    static constexpr int bytes = (cells + 7) / 8;

    /* Destruction state, one bit per panel cell: destroyed persists
     * (the collision reads it), dirty is the one-shot repaint request
     * of the frame the cell went down in. */
    uint8_t destroyed[bytes] = {};
    uint8_t dirty[bytes] = {};

    /* The tile book of the frame: touched tile cells, dedup'd; the
     * overflow is dropped - the same cell re-books the next frame. */
    static constexpr int tile_max = 32;
    uint8_t tile_r[tile_max] = {};
    uint8_t tile_c[tile_max] = {};
    int tile_n = 0;

    static bool bit_get(const uint8_t* bits, const int i);
    static void bit_set(uint8_t* bits, const int i);
    static void bit_clr(uint8_t* bits, const int i);
};

template <Disp LcdT>
void MapT<LcdT>::draw() const
{
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
        {
            const uint16_t* sprite;

            switch (at(r, c))
            {
                case MAP_BRICK: sprite = map_brick; break;
                case MAP_STEEL: sprite = map_steel; break;
                case MAP_WATER: sprite = map_water; break;
                case MAP_BUSH:  sprite = map_bush;  break;
                default:        continue;
            }

            LcdT::print(c * cell, r * cell, cell, cell, sprite);
        }
}

/* The px-rect to cell-range walk, clamped to the panel grid: shared
 * shape of blocks()/hit()/bush_touch() (each keeps its own body per
 * the house style). */
template <Disp LcdT>
bool MapT<LcdT>::blocks(const int x, const int y, const int w, const int h) const
{
    int r0 = y / cell;
    int r1 = (y + h - 1) / cell;
    int c0 = x / cell;
    int c1 = (x + w - 1) / cell;

    if (r0 < 0)     r0 = 0;
    if (c0 < 0)     c0 = 0;
    if (r1 >= rows) r1 = rows - 1;
    if (c1 >= cols) c1 = cols - 1;

    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c)
        {
            switch (at(r, c))
            {
                case MAP_STEEL:
                case MAP_WATER:
                    return true;
                case MAP_BRICK:
                    if (!bit_get(destroyed, r * cols + c))
                        return true;
                    break;
                default:
                    break;
            }
        }

    return false;
}

template <Disp LcdT>
bool MapT<LcdT>::hit(const int x, const int y, const int w, const int h)
{
    int r0 = y / cell;
    int r1 = (y + h - 1) / cell;
    int c0 = x / cell;
    int c1 = (x + w - 1) / cell;

    if (r0 < 0)     r0 = 0;
    if (c0 < 0)     c0 = 0;
    if (r1 >= rows) r1 = rows - 1;
    if (c1 >= cols) c1 = cols - 1;

    bool dead = false;

    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c)
        {
            switch (at(r, c))
            {
                case MAP_STEEL:
                    dead = true;
                    break;
                case MAP_BRICK:
                {
                    const int idx = r * cols + c;

                    /* a downed brick is emptiness: no second kill */
                    if (!bit_get(destroyed, idx))
                    {
                        bit_set(destroyed, idx);
                        bit_set(dirty, idx);

                        dead = true;
                    }

                    break;
                }
                default:
                    break;
            }
        }

    return dead;
}

template <Disp LcdT>
void MapT<LcdT>::flush_destroyed()
{
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
        {
            const int idx = r * cols + c;

            if (!bit_get(dirty, idx))
                continue;

            bit_clr(dirty, idx);

            if (at(r, c) == MAP_BRICK)
                LcdT::print(c * cell, r * cell, cell, cell, img_clear);
        }
}

template <Disp LcdT>
void MapT<LcdT>::tile_touch(const int x, const int y, const int w, const int h)
{
    int r0 = y / cell;
    int r1 = (y + h - 1) / cell;
    int c0 = x / cell;
    int c1 = (x + w - 1) / cell;

    if (r0 < 0)     r0 = 0;
    if (c0 < 0)     c0 = 0;
    if (r1 >= rows) r1 = rows - 1;
    if (c1 >= cols) c1 = cols - 1;

    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c)
        {
            switch (at(r, c))
            {
                case MAP_BRICK:
                    if (bit_get(destroyed, r * cols + c))
                        continue;
                    break;
                case MAP_STEEL:
                case MAP_WATER:
                case MAP_BUSH:
                    break;
                default:
                    continue;
            }

            bool seen = false;

            for (int i = 0; i < tile_n && !seen; ++i)
                seen = (tile_r[i] == (uint8_t)r && tile_c[i] == (uint8_t)c);

            if (!seen && (tile_n < tile_max))
            {
                tile_r[tile_n] = (uint8_t)r;
                tile_c[tile_n] = (uint8_t)c;
                tile_n += 1;
            }
        }
}

template <Disp LcdT>
void MapT<LcdT>::tile_flush()
{
    for (int i = 0; i < tile_n; ++i)
    {
        const int r = tile_r[i];
        const int c = tile_c[i];

        const uint16_t* sprite;

        switch (at(r, c))
        {
            case MAP_BRICK:
                if (bit_get(destroyed, r * cols + c))
                    continue;
                sprite = map_brick;
                break;
            case MAP_STEEL: sprite = map_steel; break;
            case MAP_WATER: sprite = map_water; break;
            case MAP_BUSH:  sprite = map_bush;  break;
            default:        continue;
        }

        LcdT::print(c * cell, r * cell, cell, cell, sprite);
    }

    tile_n = 0;
}

template <Disp LcdT>
bool MapT<LcdT>::bit_get(const uint8_t* bits, const int i)
{
    return (bits[i >> 3] >> (i & 7)) & 1;
}

template <Disp LcdT>
void MapT<LcdT>::bit_set(uint8_t* bits, const int i)
{
    bits[i >> 3] |= (uint8_t)(1u << (i & 7));
}

template <Disp LcdT>
void MapT<LcdT>::bit_clr(uint8_t* bits, const int i)
{
    bits[i >> 3] &= (uint8_t)(~(1u << (i & 7)));
}
