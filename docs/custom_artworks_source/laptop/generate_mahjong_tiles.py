#!/usr/bin/env python3
"""Generate the mahjong tile sheets for the Arulco Mahjong Club laptop page.

Classic tile look: soft white tile with bevel and rounded corners; man tiles
carry a black Chinese numeral with a small red digit, sou tiles bamboo sticks
(red centre stick, bird for the 1), pin tiles ring-style circles.

Draws 27 Sichuan tile faces plus a tile back and writes them as indexed,
ETRLE-compressed multi-sub-image STI files, the format JA2 loads natively
(see src/sgp/ImgFmt.h).

Output (relative to the repo root):
    assets/externalized/sti/laptop/mahjongtiles.sti       28 sub-images, 30x40
    assets/externalized/sti/laptop/mahjongtilessmall.sti  28 sub-images, 20x27

Run from anywhere:  python3 generate_mahjong_tiles.py
No anti-aliasing anywhere - the palette stays tiny and edges stay crisp.
"""

import math
import struct
from pathlib import Path

from PIL import Image, ImageDraw

REPO = Path(__file__).resolve().parents[3]
OUT_DIR = REPO / "assets" / "externalized" / "sti" / "laptop"

# palette: index 0 is transparent by convention
PALETTE = [
    (0, 0, 0),        # 0 transparent
    (250, 250, 246),  # 1 tile face white
    (192, 192, 184),  # 2 edge gray
    (168, 168, 160),  # 3 shadow gray
    (255, 255, 255),  # 4 highlight
    (38, 38, 38),     # 5 ink black
    (208, 64, 56),    # 6 red
    (46, 139, 60),    # 7 green
    (31, 107, 46),    # 8 dark green
    (46, 107, 192),   # 9 blue
    (46, 155, 139),   # 10 teal
    (16, 96, 68),     # 11 back felt
    (242, 202, 196),  # 12 voided face (red tint)
    (206, 148, 142),  # 13 voided edge
    (182, 124, 118),  # 14 voided shadow
    (252, 228, 224),  # 15 voided highlight
    (16, 84, 44),     # 16 felt base
    (13, 76, 39),     # 17 felt dark speckle
    (20, 92, 50),     # 18 felt light speckle
    (25, 100, 56),    # 19 felt fleck
    (255, 214, 64),   # 20 neon yellow (logo)
    (110, 26, 18),    # 21 neon halo dark red (logo)
    (98, 102, 98),    # 22 static mid gray
    (9, 34, 21),      # 23 offline feed background (dark green)
    (17, 56, 33),     # 24 offline silhouette
    (168, 40, 34),    # 25 dragon crimson
    (110, 22, 20),    # 26 dragon shadow
    (92, 20, 24),     # 27 red felt base
    (82, 16, 20),     # 28 red felt dark speckle
    (102, 26, 30),    # 29 red felt light speckle
    (114, 34, 36),    # 30 red felt fleck
    (216, 172, 64),   # 31 dragon gold
    (64, 20, 22),     # 32 dragon maroon watermark (slightly lighter than box)
    (44, 12, 14),     # 33 maroon plaque background
    (110, 40, 36),    # 34 maroon plaque frame
    (66, 66, 66),     # 35 soft ink for the numeral strokes
]
(TRANSPARENT, FACE, EDGE, SHADOW, HILITE, INK, RED, GREEN,
 DKGREEN, BLUE, TEAL, BACKFELT,
 VFACE, VEDGE, VSHADOW, VHILITE,
 FELT0, FELT1, FELT2, FELT3,
 LOGO_NEON, LOGO_HALO, STATIC_GRAY, OFFLINE_BG, OFFLINE_SIL,
 DRAGON_MAIN, DRAGON_DARK, RFELT0, RFELT1, RFELT2, RFELT3, DRAGON_GOLD,
 DRAGON_WM, PLAQUE_BG, PLAQUE_FRAME, SOFT_INK) = range(36)

# palette remap that turns a normal tile into its red-tinted "voided" twin
VOID_REMAP = {FACE: VFACE, EDGE: VEDGE, SHADOW: VSHADOW, HILITE: VHILITE}

# dice-style layouts for 1..9 marks, in a 3x3 unit grid (col, row)
GRID = {
    1: [(1, 1)],
    2: [(1, 0), (1, 2)],
    3: [(0, 0), (1, 1), (2, 2)],
    4: [(0, 0), (2, 0), (0, 2), (2, 2)],
    5: [(0, 0), (2, 0), (1, 1), (0, 2), (2, 2)],
    6: [(0, 0), (2, 0), (0, 1), (2, 1), (0, 2), (2, 2)],
    7: [(0, 0), (1, 0), (2, 0), (0, 1), (2, 1), (0, 2), (2, 2)],
    8: [(0, 0), (2, 0), (0, 1), (2, 1), (1, 0), (1, 2), (0, 2), (2, 2)],
    9: [(c, r) for r in range(3) for c in range(3)],
}

# 7-segment digit shapes for the small red digits
SEGMENTS = {
    1: "bc", 2: "abdeg", 3: "abcdg", 4: "bcfg", 5: "acdfg",
    6: "acdefg", 7: "abc", 8: "abcdefg", 9: "abcdfg",
}

# Chinese numerals as stroke lists in a normalised 0..100 x 0..100 box.
# Each stroke: (x0, y0, x1, y1). Deliberately simplified for tiny pixels.
# sans-serif numerals: strictly rectilinear where the form allows, uniform
# weight, no flicks or bent tails - gothic (heiti) rather than brush
NUMERALS = {
    1: [(10, 50, 90, 50)],
    2: [(22, 32, 78, 32), (10, 72, 90, 72)],
    3: [(22, 18, 78, 18), (28, 50, 72, 50), (10, 82, 90, 82)],
    4: [(14, 16, 14, 84), (86, 16, 86, 84), (14, 16, 86, 16), (14, 84, 86, 84),
        (40, 16, 40, 52), (62, 16, 62, 52)],
    5: [(18, 14, 82, 14), (46, 14, 46, 50), (18, 50, 82, 50),
        (62, 50, 62, 86), (8, 86, 92, 86)],
    6: [(50, 6, 50, 22), (8, 34, 92, 34), (32, 52, 22, 84), (68, 52, 78, 84)],
    7: [(10, 34, 90, 34), (48, 8, 48, 76), (48, 76, 88, 76)],
    8: [(42, 18, 24, 84), (58, 18, 76, 84)],
    9: [(32, 12, 32, 82), (12, 32, 74, 32), (74, 32, 74, 72), (74, 72, 92, 72)],
}


def tile_base(w, h, radius):
    img = Image.new("P", (w, h), TRANSPARENT)
    img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([0, 0, w - 1, h - 1], radius=radius, fill=FACE, outline=EDGE)
    # bevel: bottom/right shadow, top/left highlight
    d.line([radius, h - 2, w - 1 - radius, h - 2], fill=SHADOW)
    d.line([w - 2, radius, w - 2, h - 1 - radius], fill=SHADOW)
    d.line([radius, 1, w - 1 - radius, 1], fill=HILITE)
    d.line([1, radius, 1, h - 1 - radius], fill=HILITE)
    return img, d


def thick_line(d, x0, y0, x1, y1, color, thick):
    d.line([x0, y0, x1, y1], fill=color, width=thick)
    # square caps so strokes at this size don't look nibbled
    r = thick // 2
    for (px, py) in ((x0, y0), (x1, y1)):
        d.rectangle([px - r, py - r, px + r, py + r], fill=color)


def draw_seven_segment(d, digit, x, y, w, h, color, thick):
    a = (x, y, x + w, y + thick)
    g = (x, y + h // 2 - thick // 2, x + w, y + h // 2 + (thick + 1) // 2)
    dd = (x, y + h - thick, x + w, y + h)
    f = (x, y, x + thick, y + h // 2)
    b = (x + w - thick, y, x + w, y + h // 2)
    e = (x, y + h // 2, x + thick, y + h)
    c = (x + w - thick, y + h // 2, x + w, y + h)
    boxes = {"a": a, "b": b, "c": c, "d": dd, "e": e, "f": f, "g": g}
    for seg in SEGMENTS[digit]:
        bx = boxes[seg]
        d.rectangle([bx[0], bx[1], bx[2] - 1, bx[3] - 1], fill=color)


def draw_man(d, rank, w, h):
    # black numeral on top, small red digit below (classic wan tile)
    if w >= 30:
        nx, ny, nw, nh, thick = 5, 5, w - 10, 19, 2
        dw, dh, dt = 7, 9, 2
        dy = 27
    else:
        nx, ny, nw, nh, thick = 3, 2, w - 6, 13, 1
        dw, dh, dt = 5, 7, 1
        dy = 18
    for (x0, y0, x1, y1) in NUMERALS[rank]:
        thick_line(d,
                nx + x0 * nw // 100, ny + y0 * nh // 100,
                nx + x1 * nw // 100, ny + y1 * nh // 100, SOFT_INK, thick)
    draw_seven_segment(d, rank, (w - dw) // 2, dy, dw, dh, RED, dt)


def draw_pin(d, rank, w, h):
    # ring-style circles; the 1-pin gets one big ring with a red centre
    if w >= 30:
        area = (6, 7, w - 7, h - 9)
        r = 3
    else:
        area = (4, 5, w - 5, h - 7)
        r = 2
    ax, ay, bx, by = area
    if rank == 1:
        cx, cy = (ax + bx) // 2, (ay + by) // 2
        R = 8 if w >= 30 else 5
        d.ellipse([cx - R, cy - R, cx + R, cy + R], fill=TEAL, outline=DKGREEN)
        d.ellipse([cx - R // 2, cy - R // 2, cx + R // 2, cy + R // 2], fill=FACE)
        rr = max(1, R // 4)
        d.ellipse([cx - rr, cy - rr, cx + rr, cy + rr], fill=RED)
        return
    ring_colors = [BLUE, TEAL, DKGREEN]
    # explicit grid positions keep the circles evenly spaced at both sizes
    cols = [7, 15, 23] if w >= 30 else [4, 10, 16]
    rows = [7, 19, 31] if w >= 30 else [5, 12, 19]
    for i, (c, row) in enumerate(GRID[rank]):
        cx = cols[c]
        cy = rows[row]
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=ring_colors[i % 3])
        if r >= 3:
            d.ellipse([cx - 1, cy - 1, cx + 1, cy + 1], fill=FACE)
        else:
            d.point([cx, cy], fill=FACE)


def draw_bird(d, w, h):
    # the classic 1-sou bird, heavily simplified: red bird on a green perch
    if w >= 30:
        cx, cy = w // 2, h // 2 - 3
        d.ellipse([cx - 5, cy - 2, cx + 4, cy + 5], fill=RED)             # body
        d.polygon([(cx - 5, cy), (cx - 10, cy - 5), (cx - 4, cy - 1)], fill=RED)  # tail
        d.ellipse([cx + 1, cy - 7, cx + 7, cy - 1], fill=RED)             # head
        d.polygon([(cx + 7, cy - 5), (cx + 11, cy - 4), (cx + 7, cy - 2)], fill=DKGREEN)  # beak
        d.line([cx - 1, cy + 5, cx - 1, cy + 8], fill=INK)                # legs
        d.line([cx + 2, cy + 5, cx + 2, cy + 8], fill=INK)
        d.line([cx - 9, cy + 9, cx + 9, cy + 9], fill=GREEN, width=2)     # perch
    else:
        cx, cy = w // 2, h // 2 - 2
        d.ellipse([cx - 3, cy - 1, cx + 3, cy + 3], fill=RED)
        d.polygon([(cx - 3, cy), (cx - 6, cy - 3), (cx - 2, cy)], fill=RED)
        d.ellipse([cx + 1, cy - 4, cx + 4, cy - 1], fill=RED)
        d.point([cx + 5, cy - 3], fill=DKGREEN)
        d.line([cx - 5, cy + 5, cx + 5, cy + 5], fill=GREEN)


def draw_sou(d, rank, w, h):
    if rank == 1:
        draw_bird(d, w, h)
        return
    # bamboo sticks in dice layout; centre-column sticks are red like the
    # classic set
    if w >= 30:
        area = (7, 7, w - 8, h - 10)
        sw, sh = 3, 9
    else:
        area = (5, 4, w - 6, h - 7)
        sw, sh = 2, 6
    ax, ay, bx, by = area
    # explicit column positions: integer division clumped two columns together
    cols = [7, 14, 21] if w >= 30 else [4, 9, 14]
    for (c, row) in GRID[rank]:
        cx = cols[c]
        cy = ay + (by - ay) * row // 2
        color = RED if c == 1 and rank in (5, 7, 9) else GREEN
        d.rectangle([cx - sw // 2, cy - sh // 2, cx + sw // 2, cy + sh // 2], fill=color)
        d.line([cx - sw // 2, cy, cx + sw // 2, cy], fill=FACE)  # bamboo joint
        if sh >= 9:
            d.line([cx - sw // 2, cy - sh // 2 + 2, cx + sw // 2, cy - sh // 2 + 2], fill=DKGREEN if color == GREEN else INK)


def draw_back(d, w, h):
    d.rounded_rectangle([2, 2, w - 3, h - 3], radius=3, fill=BACKFELT)
    d.rounded_rectangle([4, 4, w - 5, h - 5], radius=2, outline=FACE)


def red_tinted(img):
    out = img.copy()
    px = out.load()
    w, h = out.size
    for y in range(h):
        for x in range(w):
            v = px[x, y]
            if v in VOID_REMAP:
                px[x, y] = VOID_REMAP[v]
    return out


def make_tiles(w, h, radius):
    # sub-images: 0..26 faces, 27 back, 28..54 red-tinted "voided" faces
    tiles = []
    for kind in range(27):
        suit, rank = kind // 9, kind % 9 + 1
        img, d = tile_base(w, h, radius)
        (draw_man, draw_pin, draw_sou)[suit](d, rank, w, h)
        tiles.append(img)
    back, d = tile_base(w, h, radius)
    draw_back(d, w, h)
    tiles.append(back)
    tiles.extend(red_tinted(t) for t in tiles[:27])
    return tiles


def etrle_encode(img):
    """ETRLE: rows of [0x80|len transparent] / [len, pixels...] runs, 0-terminated."""
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
        out.append(0)  # end of row
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
    header += struct.pack("<IH3B11x", 256, len(tiles), 8, 8, 8)  # Indexed union, 20 bytes
    header += struct.pack("<B3xI12x", 8, 0)                      # ubDepth, pad, uiAppDataSize, pad
    assert len(header) == 64, len(header)

    palette = bytearray()
    for i in range(256):
        r, g, b = PALETTE[i] if i < len(PALETTE) else (0, 0, 0)
        palette += struct.pack("<3B", r, g, b)

    subimages = bytearray()
    offset = 0
    for t, blob in zip(tiles, blobs):
        subimages += struct.pack("<IIhhHH", offset, len(blob), 0, 0, t.size[1], t.size[0])
        offset += len(blob)

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + bytes(palette) + bytes(subimages) + data)
    print(f"wrote {path} ({len(tiles)} sub-images, {len(data)} data bytes)")


def _load_logo_font(size):
    from PIL import ImageFont
    for path in (
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/System/Library/Fonts/Supplemental/Impact.ttf",
        "/Library/Fonts/Arial Bold.ttf",
    ):
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            continue
    return ImageFont.load_default()


def _neon_text(img, d, text, cx, y, font, fill, halo):
    """Hard-thresholded 'neon' lettering: dark halo behind bright face."""
    from PIL import Image as PILImage
    # render text mask at full res, no AA survives the threshold
    mask = PILImage.new("L", img.size, 0)
    from PIL import ImageDraw as PILImageDraw
    md = PILImageDraw.Draw(mask)
    bbox = md.textbbox((0, 0), text, font=font)
    tw = bbox[2] - bbox[0]
    x = cx - tw // 2 - bbox[0]
    md.text((x, y), text, font=font, fill=255)
    px = mask.load()
    out = img.load()
    w, h = img.size
    # halo: any neighbour lit -> halo colour; face: pixel lit -> fill
    for yy in range(h):
        for xx in range(w):
            if px[xx, yy] > 96:
                out[xx, yy] = fill
    for yy in range(h):
        for xx in range(w):
            if out[xx, yy] == fill:
                continue
            lit = False
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    nx, ny = xx + dx, yy + dy
                    if 0 <= nx < w and 0 <= ny < h and px[nx, ny] > 96:
                        lit = True
            if lit:
                out[xx, yy] = halo


def make_logo(w=380, h=100):
    img = Image.new("P", (w, h), TRANSPARENT)
    img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
    d = ImageDraw.Draw(img)
    # maroon plaque; the page draws the unified rounded frame around it
    d.rectangle([0, 0, w - 1, h - 1], fill=PLAQUE_BG)

    # subtle dragon watermark behind the whole sign text
    wm = make_dragon(88, DRAGON_WM)
    mask = Image.new("L", wm.size, 0)
    mp, wp = mask.load(), wm.load()
    for yy in range(wm.size[1]):
        for xx in range(wm.size[0]):
            if wp[xx, yy] != TRANSPARENT:
                mp[xx, yy] = 255
    img.paste(wm, ((w - 88) // 2, 5), mask)

    # headline; the second line is sized so both lines share the same width
    f1 = _load_logo_font(34)
    b1 = d.textbbox((0, 0), "SAN MONA", font=f1)
    w_head = b1[2] - b1[0]
    f2 = _load_logo_font(20)
    for size in range(24, 11, -1):
        f2 = _load_logo_font(size)
        b2 = d.textbbox((0, 0), "MAHJONG PARLOUR", font=f2)
        if b2[2] - b2[0] <= w_head:
            break
    _neon_text(img, d, "SAN MONA", w // 2, 6, f1, LOGO_NEON, LOGO_HALO)
    _neon_text(img, d, "MAHJONG PARLOUR", w // 2, 46, f2, FACE, LOGO_HALO)
    d.line([30, 78, w - 30, 78], fill=LOGO_NEON)
    sub = "est. 1999 - a Kingpin establishment"
    from PIL import ImageFont
    tiny = ImageFont.load_default()
    bbox = d.textbbox((0, 0), sub, font=tiny)
    d.text(((w - (bbox[2] - bbox[0])) // 2, 82), sub, font=tiny, fill=SHADOW)

    # flanking tiles hug the headline block
    tile_l = w // 2 - w_head // 2 - 42
    tile_r = w // 2 + w_head // 2 + 12
    for (kind, tx) in ((9, tile_l), (18, tile_r)):
        tile = make_tiles(30, 40, 3)[kind]
        mask = Image.new("L", tile.size, 0)
        mp, tp = mask.load(), tile.load()
        for yy in range(tile.size[1]):
            for xx in range(tile.size[0]):
                if tp[xx, yy] != TRANSPARENT:
                    mp[xx, yy] = 255
        img.paste(tile, (tx, 30), mask)
    return img


def make_static_frames(w, h, frames=3):
    """TV static for the 'video feed' portrait boxes - dead channel vibes."""
    out = []
    # colour-themed noise: deep greens with only a rare bright fleck
    shades = [INK, OFFLINE_BG, OFFLINE_BG, OFFLINE_SIL, OFFLINE_SIL,
              DKGREEN, FELT2, FELT3, GREEN, STATIC_GRAY]
    for f in range(frames):
        img = Image.new("P", (w, h), TRANSPARENT)
        img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
        px = img.load()
        state = 0xACE1 + f * 7919
        for y in range(h):
            for x in range(w):
                state = (state * 1103515245 + 12345) & 0x7FFFFFFF
                px[x, y] = shades[state % len(shades)]
        # a rolling darker scanline band, offset per frame
        band = (f * 19) % h
        for y in range(band, min(band + 6, h)):
            for x in range(w):
                if (x + y) % 2:
                    px[x, y] = INK
        out.append(img)
    # frame 3: offline feed - a dark silhouette on deep green, no one home
    img = Image.new("P", (w, h), OFFLINE_BG)
    img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
    d = ImageDraw.Draw(img)
    cx = w // 2
    hr = max(4, w * 16 // 100)                                # head radius, proportional
    hy = h * 14 // 100
    d.ellipse([cx - hr, hy, cx + hr, hy + 2 * hr], fill=OFFLINE_SIL)                    # head
    d.ellipse([cx - 2 * hr, hy + 2 * hr + h // 14, cx + 2 * hr, h + hr], fill=OFFLINE_SIL)  # shoulders
    out.append(img)
    return out


def make_chips():
    """Top-view casino chips for the score display: frame 0 house chip,
    frame 1 the gold leader chip. Classic edge-spot print reads as 'chip'
    even at 13px."""
    out = []
    for body, spot, ring in ((FACE, RED, RED), (LOGO_NEON, INK, LOGO_HALO)):
        img = Image.new("P", (13, 13), TRANSPARENT)
        img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
        d = ImageDraw.Draw(img)
        d.ellipse([0, 0, 12, 12], fill=body, outline=SHADOW)
        # six edge spots
        for (x, y) in ((5, 0), (0, 3), (10, 3), (0, 9), (10, 9), (5, 11)):
            d.rectangle([x, y, x + 2, y + 1], fill=spot)
        d.ellipse([3, 3, 9, 9], outline=ring)
        d.point([4, 2], fill=HILITE)
        out.append(img)
    return out


def make_felt(w, h, shades=None):
    """Speckled felt surface from a deterministic LCG - no PIL randomness."""
    img = Image.new("P", (w, h), TRANSPARENT)
    img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
    px = img.load()
    if shades is None:
        shades = [FELT0, FELT0, FELT0, FELT0, FELT0, FELT0, FELT1, FELT1, FELT2, FELT2, FELT3]
    state = 0x2F6E2B1
    for y in range(h):
        for x in range(w):
            state = (state * 1103515245 + 12345) & 0x7FFFFFFF
            px[x, y] = shades[state % len(shades)]
    return img


def make_dragon(size, color):
    """Circular dragon medallion: coiled serpent with jaw, horns, spikes."""
    img = Image.new("P", (size, size), TRANSPARENT)
    img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
    d = ImageDraw.Draw(img)
    c = size / 2.0
    R = size * 0.46
    lw = max(2, size // 40)

    # enclosing ring, broken where the body passes through
    for a0, a1 in ((135, 250), (315, 65)):
        d.arc([c - R, c - R, c + R, c + R], a0, a1, fill=color, width=lw)

    # body: one and a half coils, tapering, with a gentle wobble
    N = 170
    path = []
    for i in range(N):
        t = i / (N - 1.0)
        ang = math.radians(105 + t * 555)
        rad = R * (0.90 - 0.52 * t) + R * 0.05 * math.sin(t * math.pi * 3)
        path.append((c + rad * math.cos(ang), c + rad * math.sin(ang), ang, t))
    for x, y, ang, t in path:
        w = size * (0.020 + 0.052 * math.sin(min(1.0, t * 1.25) * math.pi))
        d.ellipse([x - w, y - w, x + w, y + w], fill=color)

    # dorsal spikes: sharp fins on the outer edge of the first coil
    for i in range(8, N - 40, 14):
        x, y, ang, t = path[i]
        w = size * (0.020 + 0.052 * math.sin(min(1.0, t * 1.25) * math.pi))
        base1 = (x + w * 0.9 * math.cos(ang - 0.45), y + w * 0.9 * math.sin(ang - 0.45))
        base2 = (x + w * 0.9 * math.cos(ang + 0.45), y + w * 0.9 * math.sin(ang + 0.45))
        tip = (x + (w + size * 0.075) * math.cos(ang + 0.12), y + (w + size * 0.075) * math.sin(ang + 0.12))
        d.polygon([base1, base2, tip], fill=color)

    # two clawed legs under the mid-body
    for i in (int(N * 0.30), int(N * 0.52)):
        x, y, ang, t = path[i]
        leg = math.radians(90)
        lx, ly = x + size * 0.05 * math.cos(ang + leg), y + size * 0.05 * math.sin(ang + leg)
        d.line([x, y, lx, ly], fill=color, width=lw)
        for spread in (-0.5, 0.0, 0.5):
            d.line([lx, ly, lx + size * 0.045 * math.cos(ang + leg + spread),
                    ly + size * 0.045 * math.sin(ang + leg + spread)], fill=color, width=max(1, lw - 1))

    # tail: three flame licks at the start of the path
    tx, ty, tang, _ = path[0]
    for k, spread in enumerate((-0.5, 0.0, 0.55)):
        d.line([tx, ty, tx + size * (0.10 + 0.02 * k) * math.cos(tang + math.pi * 0.9 + spread),
                ty + size * (0.10 + 0.02 * k) * math.sin(tang + math.pi * 0.9 + spread)],
               fill=color, width=lw)

    # head at the path's end: skull, open jaw, horns, whisker, eye
    hx, hy, hang, _ = path[-1]
    hr = size * 0.075
    d.ellipse([hx - hr, hy - hr, hx + hr, hy + hr], fill=color)
    fwd = hang + math.radians(80)  # snout points outward from the coil
    # upper jaw
    d.polygon([(hx + hr * 0.4 * math.cos(fwd - 1.2), hy + hr * 0.4 * math.sin(fwd - 1.2)),
               (hx + size * 0.14 * math.cos(fwd - 0.28), hy + size * 0.14 * math.sin(fwd - 0.28)),
               (hx + hr * 0.6 * math.cos(fwd + 0.1), hy + hr * 0.6 * math.sin(fwd + 0.1))], fill=color)
    # lower jaw, open
    d.polygon([(hx + hr * 0.4 * math.cos(fwd + 1.2), hy + hr * 0.4 * math.sin(fwd + 1.2)),
               (hx + size * 0.11 * math.cos(fwd + 0.55), hy + size * 0.11 * math.sin(fwd + 0.55)),
               (hx + hr * 0.5 * math.cos(fwd + 0.2), hy + hr * 0.5 * math.sin(fwd + 0.2))], fill=color)
    # backswept horns
    back = fwd + math.pi
    for horn in (-0.45, 0.35):
        d.line([hx + hr * 0.5 * math.cos(back + horn), hy + hr * 0.5 * math.sin(back + horn),
                hx + size * 0.12 * math.cos(back + horn * 1.6), hy + size * 0.12 * math.sin(back + horn * 1.6)],
               fill=color, width=lw)
    # whisker
    d.line([hx + hr * math.cos(fwd - 0.6), hy + hr * math.sin(fwd - 0.6),
            hx + size * 0.10 * math.cos(fwd - 1.1), hy + size * 0.10 * math.sin(fwd - 1.1)],
           fill=color, width=1)
    # eye: punched out
    er = max(1.5, size * 0.018)
    ex, ey = hx + hr * 0.35 * math.cos(back), hy + hr * 0.35 * math.sin(back)
    d.ellipse([ex - er, ey - er, ex + er, ey + er], fill=TRANSPARENT)
    return img


def make_void_icons(size=16):
    """Red suit glyphs for the 'void suit' indicator - no tile body."""
    out = []
    for suit in range(3):
        img = Image.new("P", (size, size), TRANSPARENT)
        img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
        d = ImageDraw.Draw(img)
        if suit == 0:
            # chars: the stacked strokes of the numeral three, reads "wan-ish"
            d.line([(1, 4), (10, 4)], fill=RED, width=2)
            d.line([(3, 8), (8, 8)], fill=RED, width=2)
            d.line([(0, 12), (11, 12)], fill=RED, width=2)
        elif suit == 1:
            # dots: the 1-pin bullseye
            d.ellipse([0, 2, 11, 13], outline=RED, width=2)
            d.ellipse([4, 6, 7, 9], fill=RED)
        else:
            # bams: three bamboo sticks
            for cx in (1, 5, 9):
                d.rectangle([cx, 3, cx + 1, 12], fill=RED)
                d.point([(cx, 7), (cx + 1, 7)], fill=TRANSPARENT)
        out.append(img)
    return out


def make_signature(w=132, h=36, color=LOGO_NEON):
    """A handwritten scrawl: big K, illegible flow, flourish underline."""
    img = Image.new("P", (w, h), TRANSPARENT)
    img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
    d = ImageDraw.Draw(img)
    # entry stroke sweeping into the K stem
    entry = [(4 + t * 0.6, 28 - t * 1.1 + 2.5 * math.sin(t / 20.0 * math.pi)) for t in range(21)]
    d.line(entry, fill=color, width=2)
    # K stem and arms, slightly drunk
    d.line([(16, 5), (12, 28)], fill=color, width=2)
    d.line([(13, 17), (26, 7)], fill=color, width=2)
    d.line([(17, 15), (28, 28)], fill=color, width=2)
    # the rest of the name collapses into confident scribble
    scrawl = []
    for t in range(61):
        tt = t / 60.0
        x = 31 + tt * 76
        y = 19 - 7 * abs(math.sin(tt * math.pi * 4.2)) + tt * 4 + 1.5 * math.sin(tt * 11)
        scrawl.append((x, y))
    d.line(scrawl, fill=color, width=2)
    # tail flourish looping back under the name
    tail = [(108 - t * 2.6, 29 + 3.2 * math.sin(t / 30.0 * math.pi * 2)) for t in range(31)]
    d.line(tail, fill=color, width=1)
    # the pen lifts with a dot
    d.ellipse([110, 12, 114, 16], fill=color)
    return img


def main():
    write_sti(OUT_DIR / "mahjongtiles.sti", make_tiles(30, 40, 3))
    write_sti(OUT_DIR / "mahjongtilessmall.sti", make_tiles(20, 27, 2))
    write_sti(OUT_DIR / "mahjongfelt.sti", [make_felt(502, 381)])
    write_sti(OUT_DIR / "mahjonglogo.sti", [make_logo()])
    # frames 0-3: 58x65 side feeds (3 static + silhouette); 4-7: 29x33 top feed
    write_sti(OUT_DIR / "mahjongchips.sti", make_chips())
    write_sti(OUT_DIR / "mahjongstatic.sti",
              make_static_frames(58, 65) + make_static_frames(29, 33))
    red_shades = [RFELT0, RFELT0, RFELT0, RFELT0, RFELT0, RFELT0,
                  RFELT1, RFELT1, RFELT2, RFELT2, RFELT3]
    write_sti(OUT_DIR / "mahjongfeltred.sti", [make_felt(502, 381, red_shades)])
    write_sti(OUT_DIR / "mahjongdragon.sti",
              [make_dragon(120, DRAGON_MAIN), make_dragon(120, DRAGON_GOLD),
               make_dragon(88, DRAGON_MAIN), make_dragon(88, DRAGON_GOLD),
               make_dragon(56, DRAGON_MAIN), make_dragon(56, DRAGON_GOLD),
               make_dragon(64, DRAGON_WM)])
    write_sti(OUT_DIR / "mahjongsign.sti", [make_signature()])
    write_sti(OUT_DIR / "mahjongvoid.sti", make_void_icons())


if __name__ == "__main__":
    main()
