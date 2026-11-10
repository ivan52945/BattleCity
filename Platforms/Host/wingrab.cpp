/* The observer's eye on the actual running window: grabs the client
 * area of the live viewer ("BattleCity live*") via PrintWindow and
 * writes it as a PNG. The window blit expanded the panel RGB565 into
 * BGRA8888 by bit replication - the top bits return it exactly, so
 * the roundtrip back to RGB565 is lossless for the game's own data.
 *
 * Build (CMake, from this dir - or the direct g++ line from the repo
 * root):
 *   cmake -S . -B build -G Ninja && cmake --build build
 *   g++ -std=c++20 -Wall -Wextra Platforms/Host/wingrab.cpp \
 *     -o Platforms/Host/wingrab.exe -lgdi32 -luser32
 * Run:
 *   wingrab.exe [outdir=out] [tag]
 * -> outdir/live_grab[tag].png
 */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "png.hpp"

static BOOL CALLBACK find_title(HWND hwnd, LPARAM lp)
{
    char title[128];

    if (GetWindowTextA(hwnd, title, (int)sizeof(title)) >= 15
        && std::strncmp(title, "BattleCity live", 15) == 0)
    {
        *(HWND*)lp = hwnd;
        return FALSE; /* found - stop the walk */
    }

    return TRUE;
}

int main(int argc, char** argv)
{
    const std::string outdir = (argc > 1) ? argv[1] : "out";
    const std::string tag = (argc > 2) ? argv[2] : "";

    HWND hwnd = nullptr;
    EnumWindows(find_title, (LPARAM)&hwnd);

    if (!hwnd)
    {
        std::printf("live window not found\n");
        return 1;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;

    if (w < 1 || h < 1)
    {
        std::printf("empty client\n");
        return 1;
    }

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h; /* top-down */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC mdc = CreateCompatibleDC(nullptr);
    HBITMAP bmp = CreateDIBSection(mdc, &bi, DIB_RGB_COLORS, &bits,
                                   nullptr, 0);

    if (!bmp || !bits)
    {
        std::printf("dib failed\n");
        return 1;
    }

    HBITMAP old = (HBITMAP)SelectObject(mdc, bmp);
    const BOOL ok = PrintWindow(hwnd, mdc,
                                PW_CLIENTONLY | PW_RENDERFULLCONTENT);
    SelectObject(mdc, old);

    if (!ok)
    {
        std::printf("PrintWindow failed\n");
        DeleteObject(bmp);
        DeleteDC(mdc);
        return 1;
    }

    /* BGRA8888 -> RGB565, exactly inverting the window blit - the
     * DIB bits stay alive until the conversion is done. The window
     * blit bit-replicates each channel upward (r5 -> r8 = r5<<3|r5>>2),
     * so the source field's top bits are the value: r8 sits at bits
     * 16..23, its 5 useful ones at 19..23 (g8 at 8..15 -> 10..15,
     * b8 at 0..7 -> 3..7). */
    std::vector<uint16_t> fb((size_t)w * h);
    const uint32_t* src = (const uint32_t*)bits;

    for (size_t i = 0; i < fb.size(); ++i)
    {
        const uint32_t px = src[i];

        const int r5 = (int)((px >> 19) & 0x1F);
        const int g6 = (int)((px >> 10) & 0x3F);
        const int b5 = (int)((px >> 3) & 0x1F);

        fb[i] = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
    }

    DeleteObject(bmp);
    DeleteDC(mdc);

    char path[256];
    std::snprintf(path, sizeof(path), "%s/live_grab%s.png",
                  outdir.c_str(), tag.c_str());

    write_png(path, w, h, 1, fb.data());
    std::printf("%s\n", path);

    return 0;
}
