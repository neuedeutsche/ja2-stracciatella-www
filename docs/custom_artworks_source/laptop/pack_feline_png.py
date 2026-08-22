#!/usr/bin/env python3
"""Pack edited PNGs back into felinecats.sti.

The Photoshop loop: `generate_feline_assets.py --png` dumps every frame
to feline_png/frame_NN.png. Edit them there (RGBA; anything with alpha
below 128 becomes transparent; keep edges hard - the format has no
partial alpha), then run this script. The palette is rebuilt from the
union of colours across all frames; 255 distinct colours is the ceiling.

Run from anywhere:  python3 pack_feline_png.py
"""

import re
import struct
from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
OUT = REPO / "assets" / "externalized" / "sti" / "laptop" / "felinecats.sti"
SRC = HERE / "feline_png"


def main():
    files = sorted(SRC.glob("frame_*.png"),
                   key=lambda f: int(re.search(r"(\d+)", f.stem).group(1)))
    if not files:
        raise SystemExit(f"no frame_NN.png files in {SRC}")

    palette = [(0, 0, 0)]  # index 0 stays transparent
    lookup = {}
    tiles = []
    for f in files:
        img = Image.open(f).convert("RGBA")
        w, h = img.size
        out = Image.new("P", (w, h), 0)
        px = img.load()
        for y in range(h):
            for x in range(w):
                r, g, b, a = px[x, y]
                if a < 128:
                    continue
                key = (r, g, b)
                idx = lookup.get(key)
                if idx is None:
                    if len(palette) >= 256:
                        raise SystemExit("more than 255 colours; flatten "
                                         "your layers harder")
                    idx = len(palette)
                    palette.append(key)
                    lookup[key] = idx
                out.putpixel((x, y), idx)
        tiles.append(out)

    def etrle(img):
        w, h = img.size
        p = img.load()
        b = bytearray()
        for y in range(h):
            x = 0
            while x < w:
                if p[x, y] == 0:
                    run = 0
                    while x < w and p[x, y] == 0 and run < 0x7F:
                        run += 1
                        x += 1
                    b.append(0x80 | run)
                else:
                    start = x
                    while x < w and p[x, y] != 0 and x - start < 0x7F:
                        x += 1
                    b.append(x - start)
                    b.extend(img.getpixel((i, y)) for i in range(start, x))
            b.append(0)
        return bytes(b)

    blobs = [etrle(t) for t in tiles]
    data = b"".join(blobs)
    w, h = tiles[0].size
    header = struct.pack("<4sIIII", b"STCI", len(data), len(data), 0,
                         0x0008 | 0x0020)
    header += struct.pack("<HH", h, w)
    header += struct.pack("<IH3B11x", 256, len(tiles), 8, 8, 8)
    header += struct.pack("<B3xI12x", 8, 0)
    assert len(header) == 64

    pal = bytearray()
    for i in range(256):
        r, g, b = palette[i] if i < len(palette) else (0, 0, 0)
        pal += struct.pack("<3B", r, g, b)

    subs = bytearray()
    off = 0
    for t, blob in zip(tiles, blobs):
        subs += struct.pack("<IIhhHH", off, len(blob), 0, 0,
                            t.size[1], t.size[0])
        off += len(blob)

    OUT.write_bytes(header + bytes(pal) + bytes(subs) + data)
    print(f"wrote {OUT} ({len(tiles)} frames, {len(palette)} colours)")


if __name__ == "__main__":
    main()
