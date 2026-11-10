/* Host visualizer of the Game Core: a framebuffer-backed fake panel
 * (the same Disp docking as the MCU drivers - the static print streams
 * RGB565 row-major into the window) plus a PNG dumper. The Game runs
 * its normal rand()-driven frames on the host, and every Nth frame
 * lands as an image file - the honest panel picture: map, tanks,
 * shells, brick destruction, the bush mask. Same headers as the MCU
 * build, same assets.cpp; no HAL anywhere on the path.
 *
 * Build (CMake, from this dir - or the direct g++ line from the repo
 * root):
 *   cmake -S . -B build -G Ninja && cmake --build build
 *   g++ -std=c++20 -Wall -Wextra \
 *     -I Game_Core/Game/Inc -I Game_Core/Utils/Inc \
 *     -I Game_Core/DeviceDrivers/Display/Inc \
 *     Platforms/Host/main.cpp Game_Core/Game/Src/assets.cpp \
 *     -o Platforms/Host/host_view.exe
 * Run (writes PNGs + GIF into the out dir, default ./out):
 *   host_view.exe [240|160|all] [frames=240] [seed=1]
 *                               [every=20] [scale=2] [gif_delay=2] [outdir=out]
 * Frame 0000 is the post-init state (the one map paint); one loop()
 * call = one frame (the 15 ms HAL_Delay pacing of the platforms is
 * the only thing the host does not reproduce). The GIF covers the
 * init paint plus the first 480 loop frames at the native panel size;
 * gif_delay = 0 disables it (delay is in 1/100 s, 2 ~= the 20 ms
 * frame pace of the F401).
 *
 * The GIF encoder roundtrips only with the decoder rule "the code
 * width grows when the table reaches 2^size" - gif_check.py is the
 * independent per-pixel verifier of the output against the PNG series.
 */
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "game.hpp"
#include "sim_lcd.hpp"
#include "png.hpp"

/* The fake panel lives in sim_lcd.hpp (shared with the live viewer);
 * the PNG writer lives in png.hpp. */

/* The contact sheet: every dumped frame of one run in one PNG - the
 * battle progression at a single glance. The cells go in dump order
 * (f0000 first), the run length pads with black. */
static void write_sheet(const char* path, int W, int H, int scale,
                        const std::vector<uint16_t>& snaps, int dumped)
{
    const int cw = W * scale;
    const int ch = H * scale;
    const int cols = 4;
    const int rows = (dumped + cols - 1) / cols;

    std::vector<uint8_t> raw((size_t)rows * ch * (cols * cw * 3 + 1), 0);
    int stride = cols * cw * 3 + 1;

    for (int n = 0; n < dumped; ++n)
    {
        const int ox = (n % cols) * cw;
        const int oy = (n / cols) * ch;
        const uint16_t* src = &snaps[(size_t)n * W * H];

        for (int y = 0; y < ch; ++y)
        {
            uint8_t* dst = &raw[(size_t)(oy + y) * stride + 1 + ox * 3];
            const uint16_t* srow = &src[(y / scale) * W];

            for (int x = 0; x < cw; ++x)
            {
                const uint16_t px = srow[x / scale];

                const int r5 = (px >> 11) & 0x1F;
                const int g6 = (px >> 5) & 0x3F;
                const int b5 = px & 0x1F;

                *dst++ = (uint8_t)((r5 << 3) | (r5 >> 2));
                *dst++ = (uint8_t)((g6 << 2) | (g6 >> 4));
                *dst++ = (uint8_t)((b5 << 3) | (b5 >> 2));
            }
        }
    }

    png_write(path, cols * cw, rows * ch, raw);
}

/* ===== GIF89a writer: the run as an animation =====
 *
 * The init paint plus the first n_frames loop frames at the native
 * panel size, one global palette collected from the window's own
 * pixels, LZW-encoded with the browser-safe code-size order (the
 * width grows when the code about to be assigned stops fitting, a
 * clear resets the table at 4096), Netscape infinite loop. The PNG
 * series stays the high-detail artifact; this one is the motion. */

static void write_gif(const char* path, int W, int H, int delay,
                      const std::vector<uint16_t>& snaps, int n_frames)
{
    if (n_frames < 1 || (size_t)n_frames * W * H != snaps.size())
        return;

    /* Palette of the window: distinct RGB565 words, indexed (the
     * lookup table keeps the scan linear; the game palette is a
     * couple of dozen entries). */
    static uint16_t pal_of[1 << 16];
    std::memset(pal_of, 0xFF, sizeof(pal_of));

    uint16_t colors[256];
    int n_colors = 0;

    for (size_t i = 0; i < snaps.size(); ++i)
    {
        const uint16_t px = snaps[i];

        if (pal_of[px] == 0xFFFF)
        {
            if (n_colors == 256)
            {
                std::fprintf(stderr, "gif: palette overflow (%d colors)\n",
                             n_colors + 1);
                return;
            }

            pal_of[px] = (uint16_t)n_colors;
            colors[n_colors] = px;
            n_colors += 1;
        }
    }

    std::FILE* f = std::fopen(path, "wb");

    if (!f)
    {
        std::perror(path);
        return;
    }

    std::fwrite("GIF89a", 1, 6, f);

    /* Logical screen: GCT present, 256 entries (2^(7+1)). */
    const uint8_t lsd[7] = {
        (uint8_t)W, (uint8_t)(W >> 8), (uint8_t)H, (uint8_t)(H >> 8),
        0xF7, 0, 0,
    };
    std::fwrite(lsd, 1, 7, f);

    uint8_t gct[768] = {};

    for (int i = 0; i < n_colors; ++i)
    {
        const uint16_t px = colors[i];

        const int r5 = (px >> 11) & 0x1F;
        const int g6 = (px >> 5) & 0x3F;
        const int b5 = px & 0x1F;

        gct[i * 3 + 0] = (uint8_t)((r5 << 3) | (r5 >> 2));
        gct[i * 3 + 1] = (uint8_t)((g6 << 2) | (g6 >> 4));
        gct[i * 3 + 2] = (uint8_t)((b5 << 3) | (b5 >> 2));
    }

    std::fwrite(gct, 1, 768, f);

    /* Netscape looping extension: loop forever. */
    const uint8_t loop_ext[19] = {
        0x21, 0xFF, 0x0B, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2',
        '.', '0', 0x03, 0x01, 0x00, 0x00, 0x00,
    };
    std::fwrite(loop_ext, 1, sizeof(loop_ext), f);

    /* LZW dictionary: (prefix << 8 | suffix) keys, open addressing. */
    static int hkey[5003];
    static int hval[5003];

    std::vector<uint8_t> idx((size_t)W * H);
    std::vector<uint8_t> lzw;

    for (int nfr = 0; nfr < n_frames; ++nfr)
    {
        const uint8_t gce[8] = {
            0x21, 0xF9, 0x04, 0x00,
            (uint8_t)(delay & 0xFF), (uint8_t)((delay >> 8) & 0xFF),
            0, 0,
        };
        std::fwrite(gce, 1, sizeof(gce), f);

        const uint8_t id[10] = {
            0x2C, 0, 0, 0, 0,
            (uint8_t)W, (uint8_t)(W >> 8), (uint8_t)H, (uint8_t)(H >> 8),
            0x00,
        };
        std::fwrite(id, 1, sizeof(id), f);
        std::fputc(8, f); /* LZW min code size (GCT is 256 wide) */

        const uint16_t* src = &snaps[(size_t)nfr * W * H];

        for (size_t i = 0; i < idx.size(); ++i)
            idx[i] = (uint8_t)pal_of[src[i]];

        lzw.clear();

        const int clear = 256;
        const int eoi = 257;
        int bitbuf = 0;
        int bitcnt = 0;
        int next = 258;
        int size = 9;

        const auto emit = [&](int code)
        {
            bitbuf |= code << bitcnt;
            bitcnt += size;

            while (bitcnt >= 8)
            {
                lzw.push_back((uint8_t)(bitbuf & 0xFF));
                bitbuf >>= 8;
                bitcnt -= 8;
            }
        };

        for (int h = 0; h < 5003; ++h)
            hkey[h] = -1;

        int ib = idx[0];
        emit(clear);

        for (size_t i = 1; i < idx.size(); ++i)
        {
            const int k = idx[i];
            const int key = (ib << 8) | k;
            int p = key % 5003;
            int hit = -1;

            for (;;)
            {
                if (hkey[p] < 0)
                    break;

                if (hkey[p] == key)
                {
                    hit = hval[p];
                    break;
                }

                if (++p == 5003)
                    p = 0;
            }

            if (hit < 0)
            {
                emit(ib);

                if (next < 4096)
                {
                    if (next >= (1 << size))
                        size += 1;

                    hkey[p] = key;
                    hval[p] = next;
                    next += 1;
                }
                else
                {
                    emit(clear);
                    next = 258;
                    size = 9;

                    for (int h = 0; h < 5003; ++h)
                        hkey[h] = -1;
                }

                ib = k;
            }
            else
                ib = hit;
        }

        emit(ib);
        emit(eoi);

        if (bitcnt > 0)
            lzw.push_back((uint8_t)(bitbuf & 0xFF));

        /* The LZW stream goes in <=255-byte sub-blocks. */
        size_t off = 0;

        while (off < lzw.size())
        {
            const size_t take = (lzw.size() - off > 255) ? 255
                                                         : lzw.size() - off;
            std::fputc((int)take, f);
            std::fwrite(&lzw[off], 1, take, f);
            off += take;
        }

        std::fputc(0, f); /* block terminator */
    }

    std::fputc(0x3B, f); /* trailer */
    std::fclose(f);
}

/* ===== One run: seed, init, frames, dumps ===== */

template <class LcdT>
static bool run_panel(const char* tag, int frames, int seed, int every,
                      int scale, int gif_delay, const std::string& outdir)
{
    std::srand((unsigned)seed);

    LcdT lcd;
    GameT<LcdT> game(lcd);

    std::printf("[%s %ux%u] seed=%d frames=%d every=%d scale=%d\n",
                tag, LcdT::w(), LcdT::h(), seed, frames, every, scale);

    game.init();

    std::vector<uint16_t> snaps;
    int dumped = 0;

    /* The GIF window: the init paint plus the first gif_max loop
     * frames, one snapshot per frame (the default run fits whole;
     * longer battles spill only into the PNG series). */
    constexpr int gif_max = 480;
    std::vector<uint16_t> gif_snaps;
    gif_snaps.insert(gif_snaps.end(), &LcdT::fb[0],
                     &LcdT::fb[LcdT::w() * LcdT::h()]);

    const auto dump = [&](int f, int frames_since)
    {
        char path[128];
        std::snprintf(path, sizeof(path), "%s/%s_f%04d.png",
                      outdir.c_str(), tag, f);

        const int bl = LcdT::blits;
        const uint32_t px = LcdT::blit_px;

        snaps.insert(snaps.end(), &LcdT::fb[0], &LcdT::fb[LcdT::w() * LcdT::h()]);
        write_png(path, LcdT::w(), LcdT::h(), scale, &LcdT::fb[0]);

        std::printf("  f%04d  blits/frame~%d  px/frame~%u  "
                    "tanks=%d shells=%d  -> %s\n",
                    f, bl / (frames_since ? frames_since : 1),
                    px / (frames_since ? frames_since : 1),
                    game.tanks.active_n(), game.shells.active_n(), path);

        LcdT::blits = 0;
        LcdT::blit_px = 0;
        dumped += 1;
    };

    dump(0, 0);

    for (int f = 1; f <= frames; ++f)
    {
        game.loop();

        if (f <= gif_max)
            gif_snaps.insert(gif_snaps.end(), &LcdT::fb[0],
                             &LcdT::fb[LcdT::w() * LcdT::h()]);

        if ((f % every) == 0 || f == frames)
        {
            const int since = (f % every) ? (f % every) : every;
            dump(f, since);
        }
    }

    std::printf("  dumped %d frames, clipped blits=%d\n",
                dumped, LcdT::clipped);

    if (dumped > 1)
    {
        char path[128];
        std::snprintf(path, sizeof(path), "%s/%s_sheet.png",
                      outdir.c_str(), tag);
        write_sheet(path, LcdT::w(), LcdT::h(), scale, snaps, dumped);
        std::printf("  sheet -> %s\n", path);
    }

    if (gif_delay > 0)
    {
        char path[128];
        std::snprintf(path, sizeof(path), "%s/%s.gif",
                      outdir.c_str(), tag);
        write_gif(path, LcdT::w(), LcdT::h(), gif_delay, gif_snaps,
                  1 + (frames < gif_max ? frames : gif_max));
        std::printf("  gif -> %s\n", path);
    }

    return lcd.initialized;
}

int main(int argc, char** argv)
{
    const char* panel = (argc > 1) ? argv[1] : "all";
    const int frames  = (argc > 2) ? std::atoi(argv[2]) : 240;
    const int seed    = (argc > 3) ? std::atoi(argv[3]) : 1;
    const int every   = (argc > 4) ? std::atoi(argv[4]) : 20;
    const int scale   = (argc > 5) ? std::atoi(argv[5]) : 2;
    const int gif_d   = (argc > 6) ? std::atoi(argv[6]) : 2;
    const std::string outdir = (argc > 7) ? argv[7] : "out";

    std::error_code ec;
    std::filesystem::create_directories(outdir, ec);

    bool ok = true;

    if (!std::strcmp(panel, "240") || !std::strcmp(panel, "all"))
        ok &= run_panel<SimLcd<240, 240>>("p240",
                                          frames, seed, every, scale, gif_d,
                                          outdir);

    if (!std::strcmp(panel, "160") || !std::strcmp(panel, "all"))
        ok &= run_panel<SimLcd<160, 80>>("p160",
                                         frames, seed, every, scale, gif_d,
                                         outdir);

    return ok ? 0 : 1;
}
