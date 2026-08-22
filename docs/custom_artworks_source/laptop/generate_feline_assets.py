#!/usr/bin/env python3
"""Generate the art for the Arulco Feline Society's page.

Same pipeline as the other laptop generators: shapes authored as vectors,
drawn at 6x supersample, majority-vote downsampled into an indexed palette
(no anti-aliased fringes) and packed into one ETRLE-compressed STI.

Output (relative to the repo root):
    assets/externalized/sti/laptop/felinecats.sti
        frames 0-9   the ten kitty-stamp poses, 76x56
        frame  10    the champion's rosette, 44x60
        frame  11    "Ch. Duchess Kalinka", a photograph, 110x84
        frame  12    the Society's paw mark, 20x20

The cats are 1994 clip art in the best sense: one confident silhouette,
a long tail that knows what it is doing, amber eyes, whiskers.

Run from anywhere:  python3 generate_feline_assets.py
Add --preview to drop a zoomed contact sheet in /tmp for eyeballing.
"""

import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

REPO = Path(__file__).resolve().parents[3]
OUT_DIR = REPO / "assets" / "externalized" / "sti" / "laptop"

PALETTE = [
    (0, 0, 0),        # 0  transparent
    (38, 32, 26),     # 1  ink: the cat itself
    (222, 168, 52),   # 2  amber: the eyes
    (214, 128, 140),  # 3  pink: nose, inner ear
    (248, 244, 232),  # 4  paper cream (whisker cutouts)
    (24, 28, 22),     # 5  photo dark
    (44, 52, 40),     # 6  photo mid
    (66, 76, 58),     # 7  photo lite
    (190, 150, 40),   # 8  rosette gold
    (150, 30, 30),    # 9  rosette red
    (110, 20, 20),    # 10 rosette red shade
    (236, 226, 200),  # 11 rosette cream centre
    (12, 10, 8),      # 12 photo black (the exhibit)
    (240, 232, 180),  # 13 photo stamp lettering
    (118, 84, 54),    # 14 wood mid
    (98, 68, 44),     # 15 wood dark
    (64, 42, 26),     # 16 wood seam
    (138, 102, 68),   # 17 wood lite
    (48, 40, 26),     # 18 paddock earth
    (74, 88, 46),     # 19 paddock grass
    (170, 140, 102),  # 20 dust mote
]
TRANSPARENT = 0
RGB_TO_IDX = {rgb: i for i, rgb in enumerate(PALETTE)}

SS = 8  # supersample factor


# --- vector helpers ---------------------------------------------------------
def canvas(w, h):
    img = Image.new("RGB", (w * SS, h * SS), PALETTE[TRANSPARENT])
    return img, ImageDraw.Draw(img)


def E(d, cx, cy, rx, ry, colour):
    d.ellipse((SS * (cx - rx), SS * (cy - ry), SS * (cx + rx),
               SS * (cy + ry)), fill=PALETTE[colour])


def P(d, pts, colour):
    d.polygon([(SS * x, SS * y) for x, y in pts], fill=PALETTE[colour])


def bez(p0, p1, p2, p3, t):
    u = 1 - t
    x = u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] \
        + t * t * t * p3[0]
    y = u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] \
        + t * t * t * p3[1]
    return x, y


def Tail(d, p0, p1, p2, p3, w0, w1, colour=1):
    """A tail is a bezier stroked with circles that thin toward the tip."""
    for i in range(0, 121):
        t = i / 120.0
        x, y = bez(p0, p1, p2, p3, t)
        r = (w0 + (w1 - w0) * t) / 2.0
        d.ellipse((SS * (x - r), SS * (y - r), SS * (x + r), SS * (y + r)),
                  fill=PALETTE[colour])


def Ears(d, cx, cy, spread, size, colour=1, droop=0):
    """Triangles rooted well inside the skull, tips flaring out. cy is
    the top of the head; the base sinks five units into it."""
    for sgn in (-1, 1):
        bx = cx + sgn * spread
        P(d, [(bx - sgn * 4.5, cy + 6),
              (bx + sgn * 3.0, cy + 4),
              (bx + sgn * (size * 0.6 + droop), cy - size + droop)],
          colour)


def Whisker(d, x0, y0, x1, y1, colour=1):
    d.line((SS * x0, SS * y0, SS * x1, SS * y1), fill=PALETTE[colour],
           width=max(1, SS - 2))


def downsample(img, w, h):
    """Majority vote per SSxSS block: smooth curves, zero fringe."""
    out = Image.new("P", (w, h), TRANSPARENT)
    out.putpalette([c for rgb in PALETTE for c in rgb]
                   + [0, 0, 0] * (256 - len(PALETTE)))
    px = img.load()
    for y in range(h):
        for x in range(w):
            counts = {}
            for sy in range(SS):
                for sx in range(SS):
                    c = px[x * SS + sx, y * SS + sy]
                    counts[c] = counts.get(c, 0) + 1
            best = max(counts.items(), key=lambda kv: kv[1])[0]
            out.putpixel((x, y), RGB_TO_IDX.get(best, TRANSPARENT))
    return out


# --- the ten poses, 76x56 each ----------------------------------------------
# Anatomy rules learned the hard way: every head gets a neck ellipse into
# the body, every body is at least two overlapping ellipses, and no tail
# tapers below two and a half units.

def side_head(d, cx, cy, r=9.5, facing=1, eye=True, ears=True, earsize=7,
              droop=0):
    if ears:
        Ears(d, cx, cy - r + 1, 4.8, earsize, droop=droop)
    E(d, cx, cy, r, r * 0.92, 1)
    E(d, cx + facing * r * 0.55, cy + 2, r * 0.55, r * 0.5, 1)  # muzzle
    if eye:
        E(d, cx + facing * 3.4, cy - 1.4, 1.7, 2.1, 2)
    Whisker(d, cx + facing * (r - 1), cy + 2,
            cx + facing * (r + 8), cy + 0.5)
    Whisker(d, cx + facing * (r - 1), cy + 4,
            cx + facing * (r + 7), cy + 5)


def Neck(d, x0, y0, x1, y1, w):
    """A thick stroke joining head to shoulders; no floating skulls."""
    Tail(d, (x0, y0), (x0, y0), (x1, y1), (x1, y1), w, w)


def pose_loaf():
    img, d = canvas(76, 56)
    E(d, 46, 41, 22, 12, 1)                     # the stern
    E(d, 30, 39, 16, 13, 1)                     # the shoulders, higher
    E(d, 38, 48, 24, 6, 1)                      # the tucked base
    E(d, 17, 50, 5.5, 4, 1)                     # the front paws, folded
    E(d, 27, 50.5, 5.5, 4, 1)                   # just proud of the loaf
    d.line((SS * 22, SS * 49, SS * 22, SS * 54), fill=PALETTE[4],
           width=SS)                            # the seam between them
    Tail(d, (66, 44), (76, 44), (72, 52), (52, 53), 5, 3)
    Neck(d, 20, 34, 26, 40, 13)
    side_head(d, 19, 28, facing=-1)
    return img


def pose_sit():
    img, d = canvas(76, 56)
    E(d, 45, 41, 14, 13, 1)                     # haunch
    E(d, 41, 34, 11, 13, 1)                     # back slope
    E(d, 36, 32, 9.5, 13, 1)                    # chest
    E(d, 35, 48, 10, 6, 1)                      # front paws
    Tail(d, (56, 51), (72, 51), (71, 33), (61, 29), 5, 2.6)
    Neck(d, 34, 22, 37, 30, 12)
    side_head(d, 33, 16, facing=-1)
    return img


def pose_sleep():
    img, d = canvas(76, 56)
    E(d, 40, 36, 22, 14, 1)                     # the curl
    E(d, 30, 40, 15, 10, 1)                     # the low shoulder
    side_head(d, 25, 33, r=8.5, facing=-1, eye=False, earsize=6)
    d.line((SS * 20, SS * 33, SS * 25, SS * 33.6), fill=PALETTE[4],
           width=SS)
    Tail(d, (58, 42), (72, 48), (54, 56), (26, 48), 5, 3)
    return img


def pose_stretch():
    img, d = canvas(76, 56)
    E(d, 53, 33, 12, 11, 1)                     # rear, up
    E(d, 44, 38, 12, 9, 1)                      # the sloping back
    E(d, 30, 45, 12, 7, 1)                      # chest to the floor
    E(d, 21, 48, 8, 5, 1)                       # front paws flat
    Tail(d, (60, 26), (70, 14), (60, 7), (52, 12), 4.5, 2.6)
    Neck(d, 17, 41, 26, 46, 11)
    side_head(d, 15, 38, r=8.5, facing=-1)
    return img


def pose_bat():
    img, d = canvas(76, 56)
    E(d, 47, 42, 13, 12, 1)                     # haunch
    E(d, 42, 33, 10, 13, 1)                     # chest
    Tail(d, (58, 51), (72, 51), (70, 35), (62, 31), 5, 2.6)
    Neck(d, 37, 24, 41, 31, 11)
    side_head(d, 36, 17, facing=-1, earsize=7)
    Tail(d, (36, 27), (28, 24), (20, 22), (14, 23), 6, 4)  # the paw
    E(d, 10, 21, 3.2, 3.2, 9)                   # the ball, mid-ruin
    return img


def pose_walk():
    img, d = canvas(76, 56)
    E(d, 40, 33, 18, 9, 1)                      # barrel
    E(d, 28, 34, 9, 9.5, 1)                     # rear mass
    E(d, 52, 32, 9, 9, 1)                       # shoulders
    for lx, ly in ((28, 52), (36, 53), (46, 52), (55, 53)):
        Tail(d, (lx, 38), (lx, 42), (lx - 1, 46), (lx - 1, ly), 5, 3.4)
    Tail(d, (23, 31), (12, 27), (9, 15), (15, 8), 4.5, 2.6)
    Neck(d, 57, 27, 52, 31, 11)
    side_head(d, 61, 23, r=9, facing=1)
    return img


def pose_groom():
    img, d = canvas(76, 56)
    E(d, 42, 39, 15, 13, 1)                     # seated mass
    E(d, 36, 33, 11, 11, 1)                     # chest lean
    Tail(d, (52, 32), (62, 17), (66, 13), (67, 12), 8, 5)  # the leg
    E(d, 67, 12, 4.5, 5, 1)                     # the foot
    Neck(d, 33, 29, 38, 34, 10)
    E(d, 32, 26, 8.5, 8, 1)                     # head to the flank
    Ears(d, 32, 18.5, 4.4, 6)
    Tail(d, (32, 50), (20, 53), (13, 47), (14, 39), 5, 2.8)
    return img


def pose_crouch():
    img, d = canvas(76, 56)
    E(d, 44, 45, 21, 8, 1)                      # flat to the floor
    E(d, 28, 45, 13, 7.5, 1)                    # shoulders, still flat
    Neck(d, 20, 42, 28, 45, 11)
    side_head(d, 17, 41, r=8.5, facing=-1, earsize=6, droop=3)
    Tail(d, (63, 46), (75, 45), (74, 52), (60, 53), 4.5, 2.6)
    return img


def pose_tail():
    img, d = canvas(76, 56)
    E(d, 37, 43, 12, 11, 1)                     # standing, tail asking
    E(d, 34, 34, 9.5, 11, 1)
    E(d, 34, 50, 9, 5, 1)
    Neck(d, 31, 24, 34, 32, 11)
    side_head(d, 30, 17, facing=1)
    Tail(d, (47, 47), (60, 44), (62, 26), (52, 19), 5, 3)
    E(d, 51, 15, 3.4, 3.4, 1)                   # the dot of the question
    Tail(d, (52, 19), (52, 17), (51, 16), (51, 15), 3, 3)
    return img


def pose_stare():
    img, d = canvas(76, 56)
    E(d, 38, 43, 13, 11, 1)                     # front on: the audit
    E(d, 38, 34, 11, 11, 1)                     # chest
    Neck(d, 38, 28, 38, 34, 14)
    E(d, 38, 23, 11, 10.5, 1)                   # head
    Ears(d, 38, 14, 6.2, 8)
    E(d, 34.5, 21.5, 2, 2.6, 2)
    E(d, 41.5, 21.5, 2, 2.6, 2)
    P(d, [(36.6, 27), (39.4, 27), (38, 28.8)], 3)
    Whisker(d, 28, 26, 16, 24)
    Whisker(d, 28, 28.5, 17, 29)
    Whisker(d, 48, 26, 60, 24)
    Whisker(d, 48, 28.5, 59, 29)
    Tail(d, (49, 50), (61, 52), (63, 42), (57, 36), 4.5, 2.6)
    return img


# --- the rosette, 44x60 -----------------------------------------------------
def rosette():
    img, d = canvas(44, 60)
    # the tails first, so the disc sits on them
    P(d, [(15, 30), (23, 32), (16, 56), (8, 52)], 9)
    P(d, [(29, 30), (21, 32), (28, 56), (36, 52)], 10)
    # pleats: a fan of gold triangles
    import math
    for i in range(16):
        a0 = i * math.tau / 16
        a1 = a0 + math.tau / 32
        P(d, [(22, 20),
              (22 + 20 * math.cos(a0), 20 + 20 * math.sin(a0)),
              (22 + 20 * math.cos(a1), 20 + 20 * math.sin(a1))], 8)
    E(d, 22, 20, 14, 14, 9)
    E(d, 22, 20, 9, 9, 11)
    # the winning number
    d.line((SS * 20, SS * 16, SS * 20, SS * 24), fill=PALETTE[10],
           width=SS * 2)
    d.line((SS * 18, SS * 18, SS * 20, SS * 16), fill=PALETTE[10],
           width=SS * 2)
    return img


# --- the photograph, 110x84 -------------------------------------------------
def _lcg(state):
    return (state * 1103515245 + 12345) & 0x7FFFFFFF


def photograph():
    img, d = canvas(110, 84)
    # grainy dark background, deterministic
    px = img.load()
    seed = 977
    for y in range(84):
        for x in range(110):
            seed = _lcg(seed)
            roll = seed % 100
            c = 5 if roll < 68 else 6 if roll < 92 else 7
            for sy in range(SS):
                for sx in range(SS):
                    px[x * SS + sx, y * SS + sy] = PALETTE[c]
    # the exhibit itself is blitted live from the game's own bloodcat
    # sprite. Here: the paddock it is photographed in - earth, grass,
    # one fence post with wire - and the burned-in date stamp
    P(d, [(0, 60), (110, 56), (110, 84), (0, 84)], 18)  # the ground
    seed2 = 411
    for i in range(46):
        seed2 = _lcg(seed2)
        gx = seed2 % 108
        seed2 = _lcg(seed2)
        gy = 60 + (seed2 % 22)
        d.line((SS * gx, SS * gy, SS * (gx + 1), SS * (gy - 3)),
               fill=PALETTE[19], width=SS - 3)
        d.line((SS * (gx + 1.5), SS * gy, SS * (gx + 2.5), SS * (gy - 2)),
               fill=PALETTE[19], width=SS - 3)
    # the fence: one post, two wires, traditional husbandry
    P(d, [(98, 30), (103, 30), (102, 78), (97, 78)], 16)
    d.line((0, SS * 40, SS * 110, SS * 36), fill=PALETTE[16], width=SS - 2)
    d.line((0, SS * 52, SS * 110, SS * 48), fill=PALETTE[16], width=SS - 2)
    d.line((SS * 88, SS * 78, SS * 105, SS * 78), fill=PALETTE[5],
           width=SS * 6)
    for i, seg in enumerate("03 97"):
        if seg == " ":
            continue
        d.line((SS * (89 + i * 4), SS * 76, SS * (91 + i * 4), SS * 80),
               fill=PALETTE[13], width=SS)
    return img


# --- the club wall: vertical planks, 500x400 --------------------------------
def wood(w=500, h=400):
    img = Image.new("P", (w, h), 14)
    img.putpalette([c for rgb in PALETTE for c in rgb]
                   + [0, 0, 0] * (256 - len(PALETTE)))
    px = img.load()
    seed = 733
    plank = 46
    for x in range(w):
        # each plank carries its own grain phase
        col = x // plank
        seam = x % plank in (0, 1)
        for y in range(h):
            seed = _lcg(seed)
            roll = (seed >> 7) % 100
            if seam:
                c = 16
            else:
                # slow vertical grain: streaks keyed to column + y band
                streak = ((x * 7 + (y // 3) * (3 + col % 4)) // 5) % 9
                c = 15 if streak in (0, 1) else 17 if streak == 4 else 14
                if roll < 4:
                    c = 15
                elif roll > 96:
                    c = 17
            px[x, y] = c
    # dust and specks: the club is cleaned before shows only
    seed = 517
    for i in range(340):
        seed = _lcg(seed)
        dx = seed % w
        seed = _lcg(seed)
        # dust settles low: bias the vertical roll toward the floor
        dy = h - 1 - int(((seed % 1000) / 1000.0) ** 2 * h)
        seed = _lcg(seed)
        kind = seed % 10
        if kind < 6:
            px[dx, dy] = 20
        elif kind < 8:
            px[dx, dy] = 17
            if dx + 1 < w:
                px[dx + 1, dy] = 20
        else:
            px[dx, dy] = 16
    # a few knots, deterministic
    for kx, ky in ((78, 90), (214, 260), (356, 150), (452, 330), (120, 320)):
        for dy in range(-4, 5):
            for dx in range(-6, 7):
                if dx * dx / 36.0 + dy * dy / 16.0 <= 1.0:
                    px[kx + dx, ky + dy] = 15
        for dy in range(-2, 3):
            for dx in range(-3, 4):
                if dx * dx / 9.0 + dy * dy / 4.0 <= 1.0:
                    px[kx + dx, ky + dy] = 16
    return img


# --- the paw mark, 20x20 ----------------------------------------------------
def paw():
    img, d = canvas(20, 20)
    E(d, 10, 13, 5.5, 4.5, 1)
    E(d, 4.5, 7, 2.2, 2.8, 1)
    E(d, 8.5, 4.5, 2.2, 2.8, 1)
    E(d, 12.5, 4.5, 2.2, 2.8, 1)
    E(d, 16, 7, 2.2, 2.8, 1)
    return img


# --- the NEW!! burst, 36x36 -------------------------------------------------
def burst():
    import math
    img, d = canvas(36, 36)
    for i in range(8):
        a = i * math.tau / 8
        P(d, [(18 + 17 * math.cos(a), 18 + 17 * math.sin(a)),
              (18 + 9 * math.cos(a + math.tau / 16),
               18 + 9 * math.sin(a + math.tau / 16)),
              (18 + 9 * math.cos(a - math.tau / 16),
               18 + 9 * math.sin(a - math.tau / 16))], 9)
    E(d, 18, 18, 11, 11, 9)
    E(d, 18, 18, 9.5, 9.5, 8)
    return img


# --- STI packing (same layout the other generators emit) --------------------
def etrle_encode(img):
    w, h = img.size
    px = img.load()
    out = bytearray()
    for y in range(h):
        x = 0
        while x < w:
            if px[x, y] == TRANSPARENT:
                run = 0
                while x < w and px[x, y] == TRANSPARENT and run < 0x7F:
                    run += 1
                    x += 1
                out.append(0x80 | run)
            else:
                start = x
                while x < w and px[x, y] != TRANSPARENT and x - start < 0x7F:
                    x += 1
                out.append(x - start)
                out.extend(img.getpixel((i, y)) for i in range(start, x))
        out.append(0)
    return bytes(out)


def write_sti(path, tiles):
    w, h = tiles[0].size
    blobs = [etrle_encode(t) for t in tiles]
    data = b"".join(blobs)

    STCI_INDEXED = 0x0008
    STCI_ETRLE_COMPRESSED = 0x0020

    header = struct.pack("<4sIIII", b"STCI", len(data), len(data), 0,
                         STCI_INDEXED | STCI_ETRLE_COMPRESSED)
    header += struct.pack("<HH", h, w)
    header += struct.pack("<IH3B11x", 256, len(tiles), 8, 8, 8)
    header += struct.pack("<B3xI12x", 8, 0)
    assert len(header) == 64, len(header)

    palette = bytearray()
    for i in range(256):
        r, g, b = PALETTE[i] if i < len(PALETTE) else (0, 0, 0)
        palette += struct.pack("<3B", r, g, b)

    subimages = bytearray()
    offset = 0
    for t, blob in zip(tiles, blobs):
        subimages += struct.pack("<IIhhHH", offset, len(blob), 0, 0,
                                 t.size[1], t.size[0])
        offset += len(blob)

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + bytes(palette) + bytes(subimages) + data)
    print(f"wrote {path} ({len(tiles)} sub-images, {len(data)} data bytes)")


def main():
    poses = [pose_loaf(), pose_sit(), pose_sleep(), pose_stretch(),
             pose_bat(), pose_walk(), pose_groom(), pose_crouch(),
             pose_tail(), pose_stare()]
    frames = [downsample(p, 76, 56) for p in poses]
    frames.append(downsample(rosette(), 44, 60))
    frames.append(downsample(photograph(), 110, 84))
    frames.append(downsample(paw(), 20, 20))
    frames.append(downsample(burst(), 36, 36))
    frames.append(wood())
    write_sti(OUT_DIR / "felinecats.sti", frames)

    if "--png" in sys.argv:
        # a Photoshop-editable dump: one RGBA PNG per frame. Edit, keep
        # hard pixel edges (no soft anti-aliasing into transparency),
        # then run pack_feline_png.py to rebuild the STI.
        outdir = Path(__file__).resolve().parent / "feline_png"
        outdir.mkdir(exist_ok=True)
        for i, f in enumerate(frames):
            rgba = Image.new("RGBA", f.size, (0, 0, 0, 0))
            fp = f.load()
            for y in range(f.size[1]):
                for x in range(f.size[0]):
                    idx = fp[x, y]
                    if idx != TRANSPARENT:
                        rgba.putpixel((x, y), PALETTE[idx] + (255,))
            rgba.save(outdir / f"frame_{i:02d}.png")
        print(f"wrote {len(frames)} PNGs to {outdir}")

    if "--preview" in sys.argv:
        zoom = 4
        cols = 5
        rows = (len(frames) + cols - 1) // cols
        sheet = Image.new("RGB", (cols * 116 * zoom, rows * 90 * zoom),
                          PALETTE[4])
        for i, f in enumerate(frames):
            rgb = Image.new("RGB", f.size, PALETTE[4])
            fp = f.load()
            for y in range(f.size[1]):
                for x in range(f.size[0]):
                    idx = fp[x, y]
                    if idx != TRANSPARENT:
                        rgb.putpixel((x, y), PALETTE[idx])
            big = rgb.resize((f.size[0] * zoom, f.size[1] * zoom),
                             Image.NEAREST)
            sheet.paste(big, ((i % cols) * 116 * zoom + 8,
                              (i // cols) * 90 * zoom + 8))
        out = Path("/tmp/feline_preview.png")
        sheet.save(out)
        print(f"wrote {out}")


if __name__ == "__main__":
    main()
