#pragma once

/* The fake panel shared by the host tools (batch dump and the live
 * viewer): a framebuffer under the Disp contract - the same docking
 * the MCU drivers satisfy. The print copies the caller's pixels into
 * static per-instantiation RAM exactly like SetWindow+DMA would
 * stream them (row-major into the window). Coordinates that wrapped
 * past zero (an entity trail band at a field edge may go x-2 < 0
 * before the uint16_t cast) paint nothing - the guarded sampling
 * keeps the field clean instead of reproducing the panel RAM wrap. */
#include <cstdint>
#include <cstring>

template <uint16_t W, uint16_t H>
class SimLcd {
public:
    void init()
    {
        std::memset(fb, 0, sizeof(fb));
        initialized = true;
    }

    void select() const {}
    void unselect() const {}

    static void print(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      const uint16_t img[])
    {
        blits += 1;
        blit_px += (uint32_t)w * h;

        const int x0 = (x > W) ? W : x; /* a wrapped coord clips away */
        const int y0 = (y > H) ? H : y;
        int x1 = x0 + w;
        int y1 = y0 + h;
        if (x1 > W) { x1 = W; }
        if (y1 > H) { y1 = H; }

        /* An empty rect after the clamp paints nothing (the edge
         * trail bands that wrapped past zero, and the bands pushed
         * off the far edge) - accounted, never shown. */
        if (x0 >= x1 || y0 >= y1)
        {
            clipped += 1;
            return;
        }

        for (int r = y0; r < y1; ++r)
        {
            const int ir = (int)r - (int)y;

            if (ir < 0 || ir >= h)
                continue;

            for (int c = x0; c < x1; ++c)
            {
                const int ic = (int)c - (int)x;

                if (ic >= 0 && ic < w)
                {
                    /* The assets are authored in the panel byte order
                     * (the 8-bit SPI streams the LE halfword LSB-first
                     * and the panel latches the first byte as the high
                     * one) - the framebuffer holds the true RGB565
                     * word, so the stream swaps back here. */
                    const uint16_t px = img[ir * w + ic];

                    fb[r * W + c] = (uint16_t)((px << 8) | (px >> 8));
                }
            }
        }
    }

    static constexpr uint16_t w() { return W; }
    static constexpr uint16_t h() { return H; }

    static inline uint16_t fb[H * W];
    static inline int blits = 0;
    static inline uint32_t blit_px = 0;
    static inline int clipped = 0;
    bool initialized = false;
};
