#!/usr/bin/env python3
"""Generate the five synthetic 128x128 PGM example images.

Every image is fully deterministic (fixed arithmetic patterns, no
randomness), so the files in this directory are reproducible by
re-running this script. The images are chosen to exercise different
failure modes of global equalization:

  gradient.pgm      smooth 0..255 ramp (already uniform -- HE is ~identity)
  low_contrast.pgm  values squeezed into 100..140 (HE's home turf)
  bimodal.pgm       dark left half, bright right half (HE vs CLAHE differ)
  checker.pgm       40/200 checkerboard (high contrast already)
  dark.pgm          values squeezed into 5..45 (dark-scene case)

Written as binary PGM (P5), maxval 255.
"""

import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
W = H = 128


def write_pgm(name, pixels):
    path = os.path.join(HERE, name)
    with open(path, "wb") as f:
        f.write(f"P5\n{W} {H}\n255\n".encode("ascii"))
        f.write(bytes(pixels))
    print(f"wrote {name} ({W}x{H})")


def gradient():
    return [x * 255 // 127 for y in range(H) for x in range(W)]


def low_contrast():
    # 100..140, diagonal pattern
    return [100 + ((x * 7 + y * 13) % 41) for y in range(H) for x in range(W)]


def bimodal():
    px = []
    for y in range(H):
        for x in range(W):
            if x < 64:
                px.append(20 + ((x + y) % 41))          # 20..60
            else:
                px.append(180 + ((x * 3 + y) % 41))     # 180..220
    return px


def checker():
    px = []
    for y in range(H):
        for x in range(W):
            base = 200 if ((x // 16) + (y // 16)) % 2 else 40
            px.append(base + (x % 16) // 4)             # 40..43 / 200..203
    return px


def dark():
    return [5 + ((x * 5 + y * 11) % 41) for y in range(H) for x in range(W)]


if __name__ == "__main__":
    write_pgm("gradient.pgm", gradient())
    write_pgm("low_contrast.pgm", low_contrast())
    write_pgm("bimodal.pgm", bimodal())
    write_pgm("checker.pgm", checker())
    write_pgm("dark.pgm", dark())
