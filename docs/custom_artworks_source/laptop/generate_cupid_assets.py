#!/usr/bin/env python3
"""Generate the art for Mercs & Kisses, the laptop dating site.

Same pipeline as the mahjong and chess generators: shapes authored in a 100x100
box, drawn at 6x supersample, threshold-downsampled into an indexed palette and
packed into ETRLE-compressed STI sheets. No anti-aliased fringes.

Output (relative to the repo root):
    assets/externalized/sti/laptop/cupidlogo.sti   3 frames: heart mark at 22, 14 and 40
    assets/externalized/sti/laptop/cupidicons.sti  8 frames of 14x14 site chrome
    assets/externalized/sti/laptop/cupidnight.sti  1 frame: 502x400 grainy night sky
    assets/externalized/sti/laptop/cupidpanels.sti 4 frames: rail + 3 landscape card plates

The night sky and the panel plates use the mahjong felt recipe: a
deterministic LCG picks weighted shades per pixel, so the grain is
reproducible and never anti-aliased.

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
    # the night sky: plum-black grain, stars, drifting hearts
    ( 30,  22,  32),  # 21 night base
    ( 26,  18,  28),  # 22 night deep
    ( 40,  31,  44),  # 23 night fleck
    ( 48,  38,  52),  # 24 night bright fleck
    (120, 104, 136),  # 25 star lavender
    ( 86,  40,  58),  # 26 heart adrift
    # the rail plate: warm rose, soft against the dark stage
    ( 66,  30,  42),  # 27 rail base
    ( 60,  26,  38),  # 28 rail dim
    ( 78,  40,  52),  # 29 rail fleck
    ( 52,  22,  33),  # 30 rail deep
    (150,  92, 110),  # 31 rail bevel lit
    ( 34,  14,  22),  # 32 rail bevel dark
    # the member card plate: worn maroon leather
    ( 74,  20,  28),  # 33 card base
    ( 66,  17,  25),  # 34 card dim
    ( 84,  26,  34),  # 35 card fleck
    ( 56,  13,  21),  # 36 card deep
    (118,  48,  58),  # 37 card bevel lit
    ( 38,   9,  15),  # 38 card bevel dark
    ( 14,  10,  16),  # 39 rivet ink
    (170, 150, 180),  # 40 rivet glint
]
(TRANSPARENT, HEART, HEART_SH, HEART_LN, GOLD, GOLD_SH,
 CHROME, PANEL, PANEL_UP, TEXT, TEXT_DIM,
 HOME_L, HOME_D, BROWSE_L, BROWSE_D, PROF_L, PROF_D,
 VIEW_L, VIEW_D, MAIL_L, MAIL_D,
 NIGHT0, NIGHT1, NIGHT2, NIGHT3, STAR, DRIFT,
 RAIL0, RAIL1, RAIL2, RAIL3, RAIL_HI, RAIL_LO,
 CARD0, CARD1, CARD2, CARD3, CARD_HI, CARD_LO,
 RIVET, GLINT) = range(41)

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

# the brand-size mark: outlined, shaded, glinting, pierced - a logo that
# survives being looked at
LOGO_BIG_LAYERS = [
    (_heart(s=1.08), HEART_LN),                       # dark outline
    (_heart(), HEART_SH),                             # body shade
    (_heart(s=0.86), HEART),                          # the heart itself
    ([("circle", 36, 34, 7)], TEXT),                  # the glint
    ([("circle", 38, 36, 5)], HEART),                 # glint carved back
    (ARROW, GOLD),
    ([("poly", [(80, 84), (97, 71), (99, 89)])], GOLD_SH),  # head shade
    ([("poly", [(88, 10), (92, 18), (100, 22), (92, 26), (88, 34),
                (84, 26), (76, 22), (84, 18)])], TEXT),     # one sparkle
]


def make_logo():
    return [render_layers(LOGO_LAYERS, 22), render_layers(LOGO_LAYERS, 14),
            render_layers(LOGO_BIG_LAYERS, 40)]


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
    # the lounge: a speech bubble with three thinking dots
    "viewed": [
        ([("rect", 2, 10, 98, 68, 16),
          ("poly", [(20, 64), (20, 96), (48, 66)])], VIEW_L),
        ([("circle", 28, 40, 7), ("circle", 50, 40, 7),
          ("circle", 72, 40, 7)], VIEW_D),
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


# --- textures (the mahjong felt recipe, replumbed) --------------------------
def _lcg(state):
    return (state * 1103515245 + 12345) & 0x7FFFFFFF


def _speckle(w, h, shades, seed):
    """Weighted-shade grain from a deterministic LCG - no PIL randomness."""
    img = Image.new("P", (w, h), shades[0])
    img.putpalette([v for rgb in PALETTE for v in rgb] +
                   [0] * (768 - 3 * len(PALETTE)))
    px = img.load()
    state = seed
    n = len(shades)
    for y in range(h):
        for x in range(w):
            state = _lcg(state)
            px[x, y] = shades[(state >> 16) % n]
    return img


def _blocky_heart(px, x, y, s, ink, w, h):
    """The site's 7x6 pixel heart, clipped to the sheet."""
    cells = [(0, 0, 3, 2), (4, 0, 3, 2), (0, 2, 7, 2), (1, 4, 5, 1),
             (2, 5, 3, 1)]
    for cx, cy, cw, ch in cells:
        for yy in range(ch * s):
            for xx in range(cw * s):
                tx, ty = x + cx * s + xx, y + cy * s + yy
                if 0 <= tx < w and 0 <= ty < h:
                    px[tx, ty] = ink


def make_night(w=502, h=400):
    """The wallpaper: grainy plum night with baked stars and stray hearts."""
    shades = [NIGHT0] * 7 + [NIGHT1] * 2 + [NIGHT2] * 2 + [NIGHT3]
    img = _speckle(w, h, shades, seed=0x10E5)
    px = img.load()
    state = 0x5EEDED
    for i in range(74):
        state = _lcg(state)
        x = state % w
        y = (state >> 12) % h
        ink = TEXT if i % 9 == 0 else STAR
        d = 1 + ((state >> 20) % 2)
        for yy in range(d):
            for xx in range(d):
                if x + xx < w and y + yy < h:
                    px[x + xx, y + yy] = ink
    for i in range(8):
        state = _lcg(state)
        _blocky_heart(px, state % (w - 8), (state >> 12) % (h - 7), 1,
                      DRIFT, w, h)
    return img


def _rivet(px, x, y):
    for yy in range(3):
        for xx in range(3):
            px[x + xx, y + yy] = RIVET
    px[x, y] = GLINT


def make_plate(w, h, shades, hi, lo, seed):
    """A riveted plate: grain, 1px chisel bevel, studs in the corners."""
    img = _speckle(w, h, shades, seed)
    px = img.load()
    for x in range(w):
        px[x, 0] = hi
        px[x, h - 1] = lo
    for y in range(h):
        px[0, y] = hi
        px[w - 1, y] = lo
    px[w - 1, 0] = shades[0]
    px[0, h - 1] = shades[0]
    for x, y in ((3, 3), (w - 6, 3), (3, h - 6), (w - 6, h - 6)):
        _rivet(px, x, y)
    return img


RAIL_SHADES = [RAIL0] * 7 + [RAIL1] * 2 + [RAIL2] * 2 + [RAIL3]
CARD_SHADES = [CARD0] * 7 + [CARD1] * 2 + [CARD2] * 2 + [CARD3]


def make_panels():
    """Frame 0: the nav rail. Frames 1-3: the landscape member card at its
    three heights (deck 302, ME 306, detail 300)."""
    return [
        make_plate(112, 384, RAIL_SHADES, RAIL_HI, RAIL_LO, seed=0xA110),
        make_plate(366, 302, CARD_SHADES, CARD_HI, CARD_LO, seed=0xBEA7),
        make_plate(366, 306, CARD_SHADES, CARD_HI, CARD_LO, seed=0xC0DE),
        make_plate(366, 300, CARD_SHADES, CARD_HI, CARD_LO, seed=0xD07E),
    ]


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
    write_sti(OUT_DIR / "cupidnight.sti", [make_night()])
    write_sti(OUT_DIR / "cupidpanels.sti", make_panels())

    if "--preview" in sys.argv:
        frames = make_logo() + make_icons(28)
        write_preview(frames, 28, Path("/tmp/cupid_icons_preview.png"), zoom=8)


if __name__ == "__main__":
    main()
