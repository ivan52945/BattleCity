# Pixel diff of two PNG dumps from host_view: the core-edit check.
# The same seed before/after a core change either renders identical
# (report: IDENTICAL) or the report shows how many pixels moved and
# where. Uses the independent decoder from gif_check.
import sys

from gif_check import png_decode


def main():
    if len(sys.argv) != 3:
        print('usage: pngdiff.py a.png b.png')
        return 2

    aw, ah, arows = png_decode(sys.argv[1])
    bw, bh, brows = png_decode(sys.argv[2])

    if (aw, ah) != (bw, bh):
        print(f'size mismatch: {aw}x{ah} vs {bw}x{bh}')
        return 1

    n = 0
    first = None
    minx = miny = 10 ** 9
    maxx = maxy = -1

    for y in range(ah):
        ar, br = arows[y], brows[y]

        for x in range(aw):
            o = x * 3

            if (ar[o], ar[o + 1], ar[o + 2]) != (br[o], br[o + 1],
                                                 br[o + 2]):
                n += 1
                minx = min(minx, x)
                maxx = max(maxx, x)
                miny = min(miny, y)
                maxy = max(maxy, y)

                if first is None:
                    first = (x, y, tuple(ar[o:o + 3]),
                             tuple(br[o:o + 3]))

    if n == 0:
        print('IDENTICAL')
        return 0

    print(f'diff pixels: {n} ({100.0 * n / (aw * ah):.3f}%)'
          f'  bbox: x {minx}..{maxx}, y {miny}..{maxy}')

    if first:
        print(f'first: {first[0]},{first[1]}'
              f'  {first[2]} -> {first[3]}')

    return 0


if __name__ == '__main__':
    sys.exit(main())
