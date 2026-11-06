/* Host-side compile/run check of the typed Game Core: the concept
 * Disp is satisfied by a plain fake driver class (no HAL, no MCU),
 * GameT instantiates over it and runs a few frames against it.
 * Build (host, repo root):
 *   clang++ -std=c++20 -I Game_Core/Game/Inc -I Game_Core/Utils/Inc \
 *     tmp/host_test/main.cpp Game_Core/Game/Src/assets.cpp
 */
#include <cstdint>
#include <cstdio>

#include "game.hpp"

/* Fake driver: whatever the Game Core's concept asks for, nothing
 * more. The blit path is static and just counts pixels here. */
class FakeLcd {
public:
    void init()
    {
        std::puts("[FakeLcd] init");
        initialized = true;
    }

    void select() const
    {
        std::puts("[FakeLcd] select");
    }

    void unselect() const
    {
        std::puts("[FakeLcd] unselect");
    }

    static void print_img(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                          const uint16_t img[])
    {
        (void)img;
        blits += 1;
        blit_px += (uint32_t)w * h;
        last_x = x; last_y = y;
    }

    /* Geometry as compile-time constants (constexpr, like the real
     * drivers) - the Game folds the field edges and the spawn ladder
     * from these. */
    static constexpr uint16_t w() { return 240; }
    static constexpr uint16_t h() { return 240; }

    static int blits;
    static uint32_t blit_px;
    static int last_x, last_y;
    bool initialized = false;
};

int FakeLcd::blits = 0;
uint32_t FakeLcd::blit_px = 0;
int FakeLcd::last_x = -1;
int FakeLcd::last_y = -1;

int main()
{
    FakeLcd lcd;

    /* The Game is typed to the fake: same docking as on the MCU. */
    GameT<FakeLcd> game(lcd);

    game.init();
    game.loop();
    game.loop();

    /* Field edges folded from the fake's (static constexpr) geometry. */
    static_assert(GameT<FakeLcd>::fld_x_end == 239, "field edges fold");
    static_assert(GameT<FakeLcd>::fld_y_end == 239, "field edges fold");

    std::printf("[run] blits=%u px=%u last=(%d,%d)\n",
                FakeLcd::blits, FakeLcd::blit_px,
                FakeLcd::last_x, FakeLcd::last_y);

    return (lcd.initialized && FakeLcd::blits > 0) ? 0 : 1;
}
