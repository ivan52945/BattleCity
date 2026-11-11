# The golden-frame gate of the host platform: the fixed-seed run
# must render pixel-exact against the committed reference dumps
# (golden/). Any diff means the core changed its visible behavior -
# figure out what broke, feed it back into the unit grid (a new case
# or a fixed case), and cycle again. Re-baselining is a conscious
# step: golden_check.py --rebaseline + a commit that says why.
#
# The golden run arguments are pinned: all 24 7 5 2 5 (both panels,
# 24 frames, seed 7, a dump every 5th, scale 2, gif delay 5).
import glob
import os
import shutil
import subprocess
import sys
import tempfile

from gif_check import png_decode

ARGS = ['all', '24', '7', '5', '2', '5']


def dump(dirname):
    exe = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       'build', 'host_view.exe')
    subprocess.run([exe] + ARGS + [dirname], check=True)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    golden = os.path.join(here, 'golden')

    if '--rebaseline' in sys.argv:
        shutil.rmtree(golden, ignore_errors=True)
        dump(golden)
        print('rebaselined: golden/')
        return 0

    fresh = tempfile.mkdtemp(prefix='golden_fresh_')
    try:
        dump(fresh)

        ref = sorted(os.path.basename(p)
                     for p in glob.glob(os.path.join(golden, '*.png')))
        new = sorted(os.path.basename(p)
                     for p in glob.glob(os.path.join(fresh, '*.png')))

        if ref != new:
            print('file set mismatch: golden %d vs fresh %d'
                  % (len(ref), len(new)))
            return 1

        bad = 0
        for name in ref:
            rw, rh, rrows = png_decode(os.path.join(golden, name))
            fw, fh, frows = png_decode(os.path.join(fresh, name))

            n = 0
            first = None
            for y in range(rh):
                a, b = rrows[y], frows[y]
                for o in range(0, rw * 3, 3):
                    if (a[o], a[o + 1], a[o + 2]) != (b[o], b[o + 1],
                                                      b[o + 2]):
                        n += 1
                        if first is None:
                            first = (o // 3, y)

            if n:
                print('%s: DIFF %d px (first: %d,%d)'
                      % (name, n, first[0], first[1]))
                bad += 1
            else:
                print('%s: identical' % name)

        print('GOLDEN OK' if not bad
              else 'GOLDEN BROKEN: %d files differ' % bad)
        return 0 if not bad else 1
    finally:
        shutil.rmtree(fresh, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
