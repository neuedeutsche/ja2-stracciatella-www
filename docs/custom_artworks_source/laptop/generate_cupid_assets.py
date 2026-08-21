#!/usr/bin/env python3
"""Generate the art for Mercs & Kisses, the laptop dating site.

Same pipeline as the mahjong and chess generators: shapes authored in a 100x100
box, drawn at 6x supersample, threshold-downsampled into an indexed palette and
packed into ETRLE-compressed STI sheets. No anti-aliased fringes.

Output (relative to the repo root):
    assets/externalized/sti/laptop/cupidlogo.sti   2 frames: heart mark, 22 and 14
    assets/externalized/sti/laptop/cupidicons.sti  8 frames of 14x14 site chrome

Icon frame order: home, browse, profile, viewed, mail, verified, heart, arrow.

Run from anywhere:  python3 generate_cupid_assets.py
Add --preview to drop a zoomed contact sheet in /tmp for eyeballing.
"""

import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

REPO = Path(__file__).resolve().parents[3]
OUT_DIR = REPO / "assets" / "externalized" / "sti" / "laptop"

# The site is warm where chach.com is cool: rose and cream over the same dark
# panel chrome, because Speck bought a template and changed two colours.
PALETTE = [
    (0, 0, 0),        # 0  transparent
    (214,  64,  92),  # 1  heart rose
    (161,  38,  62),  # 2  heart rose shade
    ( 96,  20,  36),  # 3  heart outline
    (245, 212, 163),  # 4  cupid gold (arrow, star)
    (196, 158, 104),  # 5  cupid gold shade
    ( 48,  46,  43),  # 6  site chrome        #302E2B
    ( 38,  37,  34),  # 7  panel              #262522
    ( 60,  59,  57),  # 8  panel raised       #3C3B39
    (232, 230, 227),  # 9  text primary
    (139, 137, 135),  # 10 text secondary
    # nav icon colours, one light/dark pair each
    (222, 120, 140),  # 11 home pink
    (170,  70,  92),  # 12 home pink dark
    (110, 170, 220),  # 13 browse blue
    ( 60, 115, 165),  # 14 browse blue dark
    (150, 200, 110),  # 15 profile green
    ( 95, 145,  60),  # 16 profile green dark
    (190, 150, 220),  # 17 viewed lilac
    (135,  95, 170),  # 18 viewed lilac dark
    (235, 200, 120),  # 19 mail cream
    (180, 140,  70),  # 20 mail cream dark
]
(TRANSPARENT, HEART, HEART_SH, HEART_LN, GOLD, GOLD_SH,
 CHROME, PANEL, PANEL_UP, TEXT, TEXT_DIM,
 HOME_L, HOME_D, BROWSE_L, BROWSE_D, PROF_L, PROF_D,
 VIEW_L, VIEW_D, MAIL_L, MAIL_D) = range(21)

SUPERSAMPLE = 6

# A plump valentine heart in the 100x100 authoring box.
def _heart(cx=50, cy=52, s=1.0):
    r = 26 * s
    return [
        ("circle", cx - r * 0.72, cy - r * 0.62, r * 0.78),
        ("circle", cx + r * 0.72, cy - r * 0.62, r * 0.78),
        ("poly", [(cx - r * 1.48, cy - r * 0.30), (cx + r * 1.48, cy - r * 0.30),
                  (cx, cy + r * 1.35)]),
    ]


def _draw_primitives(d, prims, scale, offset, fill):
    def P(x, y):
        return (offset + x * scale, offset + y * scale)

    for prim in prims:
        kind = prim[0]
        if kind == "poly":
            d.polygon([P(x, y) for x, y in prim[1]], fill=fill)
        elif kind == "rect":
            _, x0, y0, x1, y1, radius = prim
            box = [P(x0, y0), P(x1, y1)]
            if radius:
                d.rounded_rectangle(box, radius=radius * scale, fill=fill)
            else:
                d.rectangle(box, fill=fill)
        elif kind == "circle":
            _, cx, cy, r = prim
            d.ellipse([P(cx - r, cy - r), P(cx + r, cy + r)], fill=fill)
        else:
            raise ValueError(f"unknown primitive {kind!r}")


def _layer_mask(prims, size, margin):
    big = size * SUPERSAMPLE
    scale = (size - 2 * margin) * SUPERSAMPLE / 100.0
    offset = margin * SUPERSAMPLE
    mask = Image.new("L", (big, big), 0)
    _draw_primitives(ImageDraw.Draw(mask), prims, scale, offset, 255)
    small = mask.resize((size, size), Image.BOX)
    return small.point(lambda v: 255 if v >= 128 else 0)


def render_layers(layers, size, margin=1):
    """Paint (primitives, palette-index) layers in order into one frame."""
    img = Image.new("P", (size, size), TRANSPARENT)
    img.putpalette([v for rgb in PALETTE for v in rgb] +
                   [0] * (768 - 3 * len(PALETTE)))
    out = img.load()
    for prims, ink in layers:
        px = _layer_mask(prims, size, margin).load()
        for y in range(size):
            for x in range(size):
                if px[x, y]:
                    out[x, y] = ink
    return img


# --- the mark ---------------------------------------------------------------
# A heart with Speck's arrow through it, fletching high left, point low right.
ARROW = [
    ("poly", [(6, 22), (12, 16), (92, 76), (86, 82)]),        # shaft
    ("poly", [(80, 84), (97, 71), (99, 89)]),                 # head
    ("poly", [(4, 10), (18, 12), (10, 26), (2, 24)]),         # fletching
]

LOGO_LAYERS = [
    (_heart(), HEART_SH),
    (_heart(s=0.86), HEART),
    (ARROW, GOLD),
]


def make_logo():
    return [render_layers(LOGO_LAYERS, 22), render_layers(LOGO_LAYERS, 14)]


# --- site chrome icons ------------------------------------------------------
ICONS = {
    # a heart with a doorway: home is where it is
    "home": [
        (_heart(), HOME_L),
        ([("rect", 40, 58, 60, 86, 3)], HOME_D),
    ],
    # the browse grid, four member cells
    "browse": [
        ([("rect", 8, 8, 44, 44, 6), ("rect", 56, 8, 92, 44, 6),
          ("rect", 8, 56, 44, 92, 6), ("rect", 56, 56, 92, 92, 6)], BROWSE_L),
        ([("circle", 26, 24, 8), ("circle", 74, 24, 8),
          ("circle", 26, 72, 8), ("circle", 74, 72, 8)], BROWSE_D),
    ],
    # the questionnaire: a clipboard with a pencil across it
    "profile": [
        ([("rect", 14, 10, 74, 94, 6)], PROF_L),
        ([("rect", 30, 2, 58, 18, 4),
          ("rect", 24, 32, 64, 40, 0), ("rect", 24, 50, 64, 58, 0),
          ("rect", 24, 68, 52, 76, 0)], PROF_D),
        ([("poly", [(66, 92), (90, 40), (98, 44), (74, 96)])], GOLD),
    ],
    # who's viewed me: the eye
    "viewed": [
        ([("poly", [(2, 50), (50, 18), (98, 50), (50, 82)])], VIEW_L),
        ([("circle", 50, 50, 17)], VIEW_D),
        ([("circle", 50, 50, 7)], PANEL),
    ],
    # the envelope, for Speck's correspondence
    "mail": [
        ([("rect", 4, 20, 96, 84, 6)], MAIL_L),
        ([("poly", [(6, 24), (94, 24), (50, 60)])], MAIL_D),
    ],
    # the A.I.M. VERIFIED star
    "verified": [
        ([("poly", [(50, 2), (62, 36), (98, 38), (70, 60), (80, 96),
                    (50, 74), (20, 96), (30, 60), (2, 38), (38, 36)])], GOLD),
        ([("circle", 50, 50, 12)], GOLD_SH),
    ],
    # a plain small heart: match percent, likes, the usual currency
    "heart": [
        (_heart(cy=50, s=1.05), HEART_SH),
        (_heart(cy=48, s=0.9), HEART),
    ],
    # the arrow alone, for buttons that point somewhere
    "arrow": [
        (ARROW, GOLD),
        ([("poly", [(80, 84), (97, 71), (99, 89)])], GOLD_SH),
    ],
}

ICON_ORDER = ["home", "browse", "profile", "viewed", "mail",
              "verified", "heart", "arrow"]


def make_icons(size=14):
    return [render_layers(ICONS[name], size) for name in ICON_ORDER]


# --- STI packing (same layout the mahjong and chess generators emit) --------
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


def write_preview(frames, size, path, zoom=6):
    cols = len(frames)
    sheet = Image.new("RGB", (cols * (size + 2) * zoom, (size + 2) * zoom),
                      PALETTE[PANEL])
    for i, f in enumerate(frames):
        rgb = f.convert("RGB")
        big = rgb.resize((size * zoom, size * zoom), Image.NEAREST)
        sheet.paste(big, ((i * (size + 2) + 1) * zoom, zoom))
    sheet.save(path)
    print(f"wrote {path}")


def main():
    write_sti(OUT_DIR / "cupidlogo.sti", make_logo())
    write_sti(OUT_DIR / "cupidicons.sti", make_icons())

    if "--preview" in sys.argv:
        frames = make_logo() + make_icons(28)
        write_preview(frames, 28, Path("/tmp/cupid_icons_preview.png"), zoom=8)


if __name__ == "__main__":
    main()
