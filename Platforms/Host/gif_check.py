# Independent check of host_view's GIF output: decode the GIF89a with a
# pure-Python decoder written from the spec, and compare every pixel of
# every frame that has a PNG counterpart (the dumps, scale factor from
# the PNG width). GIF LZW decode follows the giflib rule: the decoder
# widens the code size once its free index exceeds 2^size.
import struct
import sys
import zlib
from pathlib import Path


def lzw_decode(data, mcs):
    clear = 1 << mcs
    eoi = clear + 1
    size = mcs + 1
    table = [bytes([i]) for i in range(clear)] + [b'', b'']
    out = bytearray()
    prev = None

    bitbuf = 0
    bitcnt = 0
    pos = 0

    def read():
        nonlocal bitbuf, bitcnt, pos, size

        while bitcnt < size:
            if pos >= len(data):
                return None
            bitbuf |= data[pos] << bitcnt
            pos += 1
            bitcnt += 8

        code = bitbuf & ((1 << size) - 1)
        bitbuf >>= size
        bitcnt -= size
        return code

    while True:
        code = read()

        if code is None or code == eoi:
            break

        if code == clear:
            size = mcs + 1
            del table[clear:]
            table += [b'', b'']  # restore the clear/eoi slots too
            prev = None
            continue

        if prev is None:
            entry = table[code]
        elif code < len(table):
            entry = table[code]
            table.append(prev + entry[:1])
        else:
            assert code == len(table), 'bad LZW code'
            entry = prev + prev[:1]
            table.append(entry)

        out += entry
        prev = entry

        if len(table) == (1 << size) and size < 12:
            size += 1

    return bytes(out)


def gif_decode(path):
    b = Path(path).read_bytes()
    assert b[:6] == b'GIF89a'
    w, h = struct.unpack('<HH', b[6:10])
    packed = b[10]
    assert packed & 0x80, 'global color table expected'
    gct_n = 2 << (packed & 7)
    p = 13
    gct = b[p:p + 3 * gct_n]
    p += 3 * gct_n

    frames = []
    delays = []

    while p < len(b):
        c = b[p]
        p += 1

        if c == 0x3B:  # trailer
            break

        if c == 0x21:  # extension
            label = b[p]
            p += 1

            if label == 0xF9:  # GCE
                n = b[p]
                assert n == 4
                delays.append(struct.unpack('<H', b[p + 2:p + 4])[0])
                p += 1 + n
                assert b[p] == 0
                p += 1
            else:  # skip sub-blocks
                while b[p] != 0:
                    p += 1 + b[p]
                p += 1

            continue

        if c == 0x2C:  # image
            left, top, iw, ih, ipacked = struct.unpack('<HHHHB', b[p:p + 9])
            p += 9
            assert (left, top) == (0, 0) and (iw, ih) == (w, h)
            assert ipacked == 0, 'no local table / interlace expected'
            mcs = b[p]
            p += 1

            data = bytearray()

            while b[p] != 0:
                n = b[p]
                p += 1
                data += b[p:p + n]
                p += n

            p += 1

            idx = lzw_decode(bytes(data), mcs)
            assert len(idx) == w * h, 'index stream length'
            frames.append((idx, gct))
            continue

        raise ValueError(f'unknown block 0x{c:02x} at {p - 1}')

    return w, h, frames, delays


def png_decode(path):
    b = Path(path).read_bytes()
    assert b[:8] == b'\x89PNG\r\n\x1a\n'
    p = 8
    w = h = None
    idat = bytearray()

    while p < len(b):
        (n,) = struct.unpack('>I', b[p:p + 4])
        typ = b[p + 4:p + 8]
        chunk = b[p + 8:p + 8 + n]

        if typ == b'IHDR':
            w, h, depth, ctype = struct.unpack('>IIBB', chunk[:10])
            assert depth == 8 and ctype == 2
        elif typ == b'IDAT':
            idat += chunk
        elif typ == b'IEND':
            break

        p += 12 + n

    raw = zlib.decompress(bytes(idat))
    stride = w * 3
    assert len(raw) == h * (stride + 1)

    rows = []

    for y in range(h):
        off = y * (stride + 1)
        assert raw[off] == 0, 'filter none expected'
        rows.append(raw[off + 1:off + 1 + stride])

    return w, h, rows


def main():
    out = Path(__file__).parent / 'out'
    ok = True

    for tag, dims in [('p240', (240, 240)), ('p160', (160, 80))]:
        w, h, frames, delays = gif_decode(out / f'{tag}.gif')
        assert (w, h) == dims, (w, h)
        assert len(frames) == len(delays), (len(frames), len(delays))
        assert set(delays) == {2}, set(delays)
        print(f'{tag}: {len(frames)} frames, delay {delays[0]}, '
              f'{w}x{h}, palette {len(frames[0][1]) // 3} entries')

        for png in sorted(out.glob(f'{tag}_f*.png')):
            k = int(png.stem.split('_f')[1])

            if k >= len(frames):
                continue  # a PNG of an older, longer run

            assert k < len(frames)
            pw, ph, rows = png_decode(png)
            scale = pw // w
            idx, gct = frames[k]

            rgb = [(gct[i * 3], gct[i * 3 + 1], gct[i * 3 + 2])
                   for i in range(256)]

            mism = 0

            for y in range(h):
                prow = rows[y * scale]
                frow = idx[y * w:(y + 1) * w]

                for x in range(w):
                    o = x * scale * 3
                    g = rgb[frow[x]]

                    if (prow[o], prow[o + 1], prow[o + 2]) != g:
                        mism += 1

            print(f'  f{k:04d} vs {png.name}: mismatches {mism}')
            ok &= (mism == 0)

    print('OK' if ok else 'FAIL')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
