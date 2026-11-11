/* Recording fake panel for host unit tests: satisfies the Disp
 * concept with a constexpr geometry fixed at compile time and a
 * static blit log (what / where was printed since the last reset).
 * The blit path is stateless by the concept, so the log is static
 * too - reset() it in every test case (and in mid-case phases).
 *
 * Every test binary is a single TU, so the static members are safe.
 */

#pragma once

#include <cstdint>
#include <vector>

#include "disp.hpp"

template <uint16_t W, uint16_t H>
class FakeLcd
{
public:
    struct Blit
    {
        uint16_t x, y, w, h;
        const uint16_t* img;
    };

    void init() { initialized = true; }
    void select() const {}
    void unselect() const {}

    static void print(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      const uint16_t img[])
    {
        log.push_back({x, y, w, h, img});
    }

    static constexpr uint16_t w() { return W; }
    static constexpr uint16_t h() { return H; }

    static void reset()
    {
        log.clear();
        initialized = false;
    }

    static std::vector<Blit> log;
    static bool initialized;
};

template <uint16_t W, uint16_t H>
bool FakeLcd<W, H>::initialized = false;

template <uint16_t W, uint16_t H>
std::vector<typename FakeLcd<W, H>::Blit> FakeLcd<W, H>::log;