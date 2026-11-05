#pragma once

#include <cstdint>
#include <concepts>

/* The docking contract of the Game Core - as a compile-time concept.
 *
 * The shape is asymmetric by design and mirrors what the drivers
 * actually are:
 *
 *   - The blit path (print_img) carries no per-object state anywhere
 *     (the transaction chain is fully parameterized by the wiring
 *     template arguments; the scratch belongs to fills only), so it
 *     is required as a STATIC member: Entity calls it with no object
 *     and no pointer, and the per-slot display pointer of the game
 *     core disappears with the pool RAM it was costing.
 *
 *   - Lifecycle and bus picking (init/select/unselect) act on the one
 *     concrete driver object the platform main() created, so they are
 *     required as instance members; GameT holds a reference to it.
 *
 *   - Geometry (w/h) is a compile-time constant of the instantiation
 *     (static constexpr geo through make_geo), required static.
 *
 * A driver qualifies by having exactly this API; a violating shape
 * fails right at the GameT instantiation point, not at three call
 * sites nearer the target.
 */
template <class LcdT>
concept Disp = requires(LcdT& lcd, uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h, const uint16_t img[]) {
    lcd.init();
    lcd.select();
    lcd.unselect();

    LcdT::print_img(x, y, w, h, img);

    { LcdT::w() } -> std::convertible_to<uint16_t>;
    { LcdT::h() } -> std::convertible_to<uint16_t>;
};
