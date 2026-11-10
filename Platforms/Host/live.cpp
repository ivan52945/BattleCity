/* Live spectator of the Game Core: the same SimLcd framebuffer panels
 * as the batch tool, blitted into a WinAPI window at the platform's
 * frame pace. Pure WinAPI - no external dependencies; the game steps
 * in the timer tick exactly like the batch tool steps it in its loop,
 * so a solo-panel run reproduces the batch battle for the same seed
 * frame for frame. In 'all' mode the two games share one rand stream
 * (the batch tool re-seeds per panel instead).
 *
 * Build (CMake, from this dir - or the direct g++ line from the repo
 * root):
 *   cmake -S . -B build -G Ninja && cmake --build build
 *   g++ -std=c++20 -Wall -Wextra \
 *     -I Game_Core/Game/Inc -I Game_Core/Utils/Inc \
 *     -I Game_Core/DeviceDrivers/Display/Inc \
 *     Platforms/Host/live.cpp Game_Core/Game/Src/assets.cpp \
 *     -o Platforms/Host/host_view_live.exe -lgdi32 -luser32
 * Run:
 *   host_view_live.exe [240|160|all] [seed=1] [scale=2] [outdir=out]
 *                      [autoframes=0]
 * Console subsystem (statistics print to the terminal; a console
 * window shows up when launched outside one - add -mwindows to strip
 * it). Keys:
 *   Space  pause / resume          S      step one frame (pauses)
 *   +/-    speed x0.5..x4          R      restart, same seed
 *   N      restart, next seed      P      dump visible panels as PNG
 *   Esc    quit
 * autoframes > 0 is a self-test hook: the run stops itself after that
 * many frames with a PNG dump (used to compare against the batch tool
 * without a keyboard in the loop).
 */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include "game.hpp"
#include "sim_lcd.hpp"
#include "png.hpp"

/* ===== Run state ===== */

static bool g_show240 = true;
static bool g_show160 = true;
static int g_scale = 2;
static int g_seed = 1;
static std::string g_outdir = "out";
static int g_autoframes = 0;

static long long g_frames = 0;
static bool g_paused = false;
static int g_speed = 1; /* 0 = x0.5, 1 = x1, 2 = x2, 3 = x4 */
static int g_half_phase = 0;

/* The games live on the heap so a restart re-creates them from a
 * fresh srand - the pools and the map cannot be reset in place. */
template <class LcdT>
struct PanelRun {
    LcdT lcd{};
    GameT<LcdT> game{lcd};
};

static PanelRun<SimLcd<240, 240>>* g_p240 = nullptr;
static PanelRun<SimLcd<160, 80>>* g_p160 = nullptr;

static void start_games()
{
    delete g_p240;
    delete g_p160;
    g_p240 = nullptr;
    g_p160 = nullptr;

    /* One srand per restart: solo mode matches the batch tool exactly,
     * 'all' mode interleaves the two games over the shared stream. */
    std::srand((unsigned)g_seed);

    if (g_show240)
    {
        g_p240 = new PanelRun<SimLcd<240, 240>>();
        g_p240->game.init();
    }

    if (g_show160)
    {
        g_p160 = new PanelRun<SimLcd<160, 80>>();
        g_p160->game.init();
    }

    g_frames = 0;
}

static void step()
{
    g_frames += 1;

    if (g_p240)
        g_p240->game.loop();

    if (g_p160)
        g_p160->game.loop();
}

static void dump_all()
{
    char path[256];

    if (g_p240)
    {
        std::snprintf(path, sizeof(path), "%s/p240_live_f%lld.png",
                      g_outdir.c_str(), g_frames);
        write_png(path, 240, 240, g_scale, g_p240->lcd.fb);
        std::printf("dump -> %s\n", path);
    }

    if (g_p160)
    {
        std::snprintf(path, sizeof(path), "%s/p160_live_f%lld.png",
                      g_outdir.c_str(), g_frames);
        write_png(path, 160, 80, g_scale, g_p160->lcd.fb);
        std::printf("dump -> %s\n", path);
    }
}

/* ===== Layout and painting ===== */

struct Layout {
    int cw, ch;
    int ox240, oy240, ox160, oy160;
};

static Layout layout()
{
    const int pad = 8;
    const int gap = 24;
    const int s = g_scale;

    Layout L{};
    L.ox240 = pad;
    L.oy240 = pad;
    L.ox160 = pad;
    L.oy160 = pad;

    int w = 0;

    if (g_show240)
    {
        L.ox240 = pad;
        w += 240 * s;
    }

    if (g_show160)
    {
        if (g_show240)
        {
            L.ox160 = pad + 240 * s + gap;
            w += gap;
        }
        else
            L.ox160 = pad;

        L.oy160 = pad;
        w += 160 * s;
    }

    L.cw = w + 2 * pad;
    L.ch = pad + 240 * s + pad; /* 240s is the taller view always */

    return L;
}

/* One panel's fb into the BGRA client buffer, nearest-neighbor. */
static void blit_panel(uint32_t* dst, int dst_w, const uint16_t* fb,
                       int W, int H, int ox, int oy, int s)
{
    for (int y = 0; y < H * s; ++y)
    {
        const uint16_t* row = &fb[(y / s) * W];
        uint32_t* d = &dst[(size_t)(oy + y) * dst_w + ox];

        for (int x = 0; x < W * s; ++x)
        {
            const uint16_t px = row[x / s];

            const int r5 = (px >> 11) & 0x1F;
            const int g6 = (px >> 5) & 0x3F;
            const int b5 = px & 0x1F;

            const uint32_t r8 = (uint32_t)((r5 << 3) | (r5 >> 2));
            const uint32_t g8 = (uint32_t)((g6 << 2) | (g6 >> 4));
            const uint32_t b8 = (uint32_t)((b5 << 3) | (b5 >> 2));

            d[x] = 0xFF000000u | (r8 << 16) | (g8 << 8) | b8;
        }
    }
}

static uint32_t* g_dib = nullptr;
static int g_dib_w = 0;
static int g_dib_h = 0;

static void dib_ensure(int cw, int ch)
{
    if (g_dib && g_dib_w == cw && g_dib_h == ch)
        return;

    delete[] g_dib;
    g_dib = new uint32_t[(size_t)cw * ch];
    g_dib_w = cw;
    g_dib_h = ch;
}

static void paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);

    RECT r;
    GetClientRect(hwnd, &r);
    const int cw = r.right - r.left;
    const int ch = r.bottom - r.top;

    if (cw > 0 && ch > 0)
    {
        dib_ensure(cw, ch);

        /* Padding in dark gray: the panels are black-backed, the
         * border stays visible. */
        for (int i = 0; i < g_dib_w * g_dib_h; ++i)
            g_dib[i] = 0xFF202020u;

        const Layout L = layout();

        if (g_p240)
            blit_panel(g_dib, g_dib_w, g_p240->lcd.fb, 240, 240,
                       L.ox240, L.oy240, g_scale);

        if (g_p160)
            blit_panel(g_dib, g_dib_w, g_p160->lcd.fb, 160, 80,
                       L.ox160, L.oy160, g_scale);

        BITMAPINFO bi{};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = g_dib_w;
        bi.bmiHeader.biHeight = -g_dib_h; /* top-down */
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        SetDIBitsToDevice(dc, 0, 0, g_dib_w, g_dib_h, 0, 0, 0, g_dib_h,
                          g_dib, &bi, DIB_RGB_COLORS);
    }

    EndPaint(hwnd, &ps);
}

/* ===== Title line: the same statistics as the batch tool ===== */

static void update_title(HWND hwnd)
{
    int b240 = 0;
    int b160 = 0;

    if (g_p240)
    {
        b240 = g_p240->lcd.blits;
        g_p240->lcd.blits = 0;
    }

    if (g_p160)
    {
        b160 = g_p160->lcd.blits;
        g_p160->lcd.blits = 0;
    }

    const char* sp[] = {"x0.5", "x1", "x2", "x4"};

    std::string s = "BattleCity live  f" + std::to_string(g_frames)
                  + "  " + sp[g_speed] + "  seed " + std::to_string(g_seed);

    if (g_p240)
        s += "  |  240: t" + std::to_string(g_p240->game.tanks.active_n())
           + " s" + std::to_string(g_p240->game.shells.active_n())
           + " b" + std::to_string(b240);

    if (g_p160)
        s += "  |  160: t" + std::to_string(g_p160->game.tanks.active_n())
           + " s" + std::to_string(g_p160->game.shells.active_n())
           + " b" + std::to_string(b160);

    if (g_paused)
        s += "  |  PAUSED (Space)";

    SetWindowTextA(hwnd, s.c_str());
}

/* ===== Window ===== */

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_CREATE:
            SetTimer(hwnd, 1, 15, NULL); /* ~ the 15 ms HAL_Delay pace */
            start_games();
            update_title(hwnd);
            return 0;

        case WM_TIMER:
            if (!g_paused)
            {
                if (g_speed == 0)
                {
                    g_half_phase ^= 1;

                    if (g_half_phase)
                        break; /* x0.5: step every other tick */

                    step();
                }
                else
                    for (int i = 0; i < (1 << (g_speed - 1)); ++i)
                        step();

                update_title(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);

                if (g_autoframes && g_frames >= g_autoframes)
                {
                    dump_all();
                    PostQuitMessage(0);
                }
            }
            break;

        case WM_PAINT:
            paint(hwnd);
            return 0;

        case WM_ERASEBKGND:
            return 1; /* the paint covers the whole client */

        case WM_KEYDOWN:
            switch (wp)
            {
                case VK_SPACE:
                    g_paused = !g_paused;
                    update_title(hwnd);
                    break;

                case 'S':
                    g_paused = true;
                    step();
                    update_title(hwnd);
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;

                case VK_OEM_PLUS:
                case VK_ADD:
                    if (g_speed < 3)
                    {
                        g_speed += 1;
                        update_title(hwnd);
                    }
                    break;

                case VK_OEM_MINUS:
                case VK_SUBTRACT:
                    if (g_speed > 0)
                    {
                        g_speed -= 1;
                        update_title(hwnd);
                    }
                    break;

                case 'R':
                    start_games();
                    update_title(hwnd);
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;

                case 'N':
                    g_seed += 1;
                    start_games();
                    std::printf("[seed] %d\n", g_seed);
                    update_title(hwnd);
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;

                case 'P':
                    dump_all();
                    break;

                case VK_ESCAPE:
                    DestroyWindow(hwnd);
                    break;
            }
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

int main(int argc, char** argv)
{
    const char* panel = (argc > 1) ? argv[1] : "all";

    if (!std::strcmp(panel, "240"))
    {
        g_show240 = true;
        g_show160 = false;
    }
    else if (!std::strcmp(panel, "160"))
    {
        g_show240 = false;
        g_show160 = true;
    }

    g_seed = (argc > 2) ? std::atoi(argv[2]) : 1;
    g_scale = (argc > 3) ? std::atoi(argv[3]) : 2;

    if (g_scale < 1)
        g_scale = 1;
    if (g_scale > 4)
        g_scale = 4;

    g_outdir = (argc > 4) ? argv[4] : "out";
    g_autoframes = (argc > 5) ? std::atoi(argv[5]) : 0;

    std::error_code ec;
    std::filesystem::create_directories(g_outdir, ec);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = wndproc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "battlecity_live";
    RegisterClassA(&wc);

    const Layout L = layout();
    RECT wr = {0, 0, L.cw, L.ch};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowA("battlecity_live", "BattleCity live",
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                              CW_USEDEFAULT, wr.right - wr.left,
                              wr.bottom - wr.top, nullptr, nullptr,
                              wc.hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOW);

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete g_p240;
    delete g_p160;

    return (int)msg.wParam;
}
