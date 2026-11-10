#pragma once

/* Minimal PNG writer shared by the host tools (batch dump and the
 * live viewer's P-key dump): truecolor 8-bit, stored deflate.
 *
 * No external dependencies: the zlib stream is uncompressed deflate
 * blocks (BTYPE=00) plus the adler32 tail - big files, zero fuss,
 * every viewer opens them. The panel RGB565 expands to RGB888 by the
 * classic bit replication, scale = nearest neighbor. */
#include <cstdint>
#include <cstdio>
#include <vector>

static uint32_t crc32_upd(uint32_t crc, const uint8_t* p, size_t n)
{
    crc = ~crc;

    for (size_t i = 0; i < n; ++i)
    {
        crc ^= p[i];

        for (int b = 0; b < 8; ++b)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0u);
    }

    return ~crc;
}

static uint32_t adler32_of(const uint8_t* p, size_t n)
{
    uint32_t a = 1;
    uint32_t b = 0;

    for (size_t i = 0; i < n; ++i)
    {
        a = (a + p[i]) % 65521;
        b = (b + a) % 65521;
    }

    return (b << 16) | a;
}

static void png_chunk(std::FILE* f, const char* type,
                      const uint8_t* data, size_t n)
{
    const uint32_t len = (uint32_t)n;
    const uint8_t lb[4] = {
        (uint8_t)(len >> 24), (uint8_t)(len >> 16),
        (uint8_t)(len >> 8),  (uint8_t)len,
    };

    std::fwrite(lb, 1, 4, f);
    std::fwrite(type, 1, 4, f);
    if (n != 0)
        std::fwrite(data, 1, n, f);

    uint32_t crc = crc32_upd(0, (const uint8_t*)type, 4);

    if (n != 0)
        crc = crc32_upd(crc, data, n);

    const uint8_t cb[4] = {
        (uint8_t)(crc >> 24), (uint8_t)(crc >> 16),
        (uint8_t)(crc >> 8),  (uint8_t)crc,
    };

    std::fwrite(cb, 1, 4, f);
}

static void png_write(const char* path, int ow, int oh,
                      const std::vector<uint8_t>& raw)
{
    std::FILE* f = std::fopen(path, "wb");

    if (!f)
    {
        std::perror(path);
        return;
    }

    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    std::fwrite(sig, 1, 8, f);

    const uint8_t ihdr[13] = {
        (uint8_t)(ow >> 24), (uint8_t)(ow >> 16), (uint8_t)(ow >> 8), (uint8_t)ow,
        (uint8_t)(oh >> 24), (uint8_t)(oh >> 16), (uint8_t)(oh >> 8), (uint8_t)oh,
        8, 2, 0, 0, 0,
    };

    png_chunk(f, "IHDR", ihdr, sizeof(ihdr));

    std::vector<uint8_t> idat;
    idat.push_back(0x78); /* zlib: 32K window, fastest */
    idat.push_back(0x01);

    size_t off = 0;

    while (off < raw.size())
    {
        const size_t take = (raw.size() - off > 65535) ? 65535
                                                       : raw.size() - off;
        const bool last = (off + take == raw.size());

        idat.push_back(last ? 1 : 0); /* BFINAL | BTYPE=00 (stored) */
        idat.push_back((uint8_t)(take & 0xFF));
        idat.push_back((uint8_t)((take >> 8) & 0xFF));

        const uint16_t nlen = (uint16_t)(~take);
        idat.push_back((uint8_t)(nlen & 0xFF));
        idat.push_back((uint8_t)(nlen >> 8));

        idat.insert(idat.end(), raw.begin() + (long)off,
                    raw.begin() + (long)(off + take));
        off += take;
    }

    const uint32_t ad = adler32_of(raw.data(), raw.size());
    const uint8_t ab[4] = {
        (uint8_t)(ad >> 24), (uint8_t)(ad >> 16),
        (uint8_t)(ad >> 8),  (uint8_t)ad,
    };
    idat.insert(idat.end(), ab, ab + 4);

    png_chunk(f, "IDAT", idat.data(), idat.size());
    png_chunk(f, "IEND", nullptr, 0);
    std::fclose(f);
}

/* The panel frame: the RGB565 RAM scaled up by the bit-replicated
 * RGB565 -> RGB888 and the nearest-neighbor scale. */
static void write_png(const char* path, int W, int H, int scale,
                      const uint16_t* fb)
{
    const int ow = W * scale;
    const int oh = H * scale;

    std::vector<uint8_t> raw;
    raw.reserve((size_t)oh * (ow * 3 + 1));

    for (int y = 0; y < oh; ++y)
    {
        raw.push_back(0); /* filter: none */

        const uint16_t* row = &fb[(y / scale) * W];

        for (int x = 0; x < ow; ++x)
        {
            const uint16_t px = row[x / scale];

            const int r5 = (px >> 11) & 0x1F;
            const int g6 = (px >> 5) & 0x3F;
            const int b5 = px & 0x1F;

            raw.push_back((uint8_t)((r5 << 3) | (r5 >> 2)));
            raw.push_back((uint8_t)((g6 << 2) | (g6 >> 4)));
            raw.push_back((uint8_t)((b5 << 3) | (b5 >> 2)));
        }
    }

    png_write(path, ow, oh, raw);
}
