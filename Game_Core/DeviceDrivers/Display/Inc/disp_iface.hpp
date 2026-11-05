#pragma once

#include <cstdint>

/* Abstract display interface of the Game Core.
 *
 * A concrete device driver (e.g. ST7789, ILI9341, ...) provides an
 * implementation of this interface. This module holds no binding
 * helpers and no globals: the platform layer creates the driver
 * instance in main() and injects it into the Game, Game::init()
 * powers the panel up and hands the geometry over to the game side.
 *
 * The Game frames every drawing cycle (each Game::loop() call) with
 * select()/unselect(): the bus is picked before the frame and freed
 * after it. Buses without a wired CS get no-ops (default bodies); a
 * single-device bus driver may hold the line asserted forever.
 */
class Disp_Iface
{
public:
    virtual ~Disp_Iface() = default;

    /* Power up the display and clear it. */
    virtual void init() = 0;

    /* Panel geometry, valid after init(). */
    virtual uint16_t w() const = 0;
    virtual uint16_t h() const = 0;

    /* Blit a 16-bit(RGB565) image rect at (x, y). */
    virtual void print_img(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t img[]) = 0;

    /* Bus arbitration around a drawing cycle: picks the device at the
     * start of the cycle, releases it at the end. No CS wired - both
     * defaults are no-ops; the driver also decides itself how to treat
     * a single-device bus. */
    virtual void select() const {}
    virtual void unselect() const {}
};
