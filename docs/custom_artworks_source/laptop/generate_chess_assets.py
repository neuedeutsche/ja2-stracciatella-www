#!/usr/bin/env python3
"""Generate the piece sheets for the chach.com laptop page.

Faithful port of chess.com's current "Neo" piece set: chunky flat silhouettes,
a single dark inner outline, one shade band under the base. No gradients, no
anti-aliased fringes - shapes are drawn at 6x and threshold-downsampled so the
edges stay crisp in an indexed palette.

Output (relative to the repo root):
    assets/externalized/sti/laptop/chesspieces.sti       12 sub-images, 34x34
    assets/externalized/sti/laptop/chesspiecessmall.sti  12 sub-images, 20x20

Frame order in both sheets: white P N B R Q K, then black P N B R Q K.

Run from anywhere:  python3 generate_chess_assets.py
Add --preview to also drop a zoomed contact sheet in /tmp for eyeballing.
"""

import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

REPO = Path(__file__).resolve().parents[3]
OUT_DIR = REPO / "assets" / "externalized" / "sti" / "laptop"

# chess.com's current palette, sampled from the live site.
# Index 0 is transparent by convention (see the mahjong generator).
PALETTE = [
    (0, 0, 0),        # 0  transparent
    (250, 250, 250),  # 1  white piece fill
    (206, 205, 203),  # 2  white piece shade (under the base)
    (92, 90, 86),     # 3  white piece outline (Neo's is grey, not black)
    (64, 61, 57),     # 4  black piece fill
    (44, 42, 39),     # 5  black piece shade
    (26, 25, 23),     # 6  black piece outline
    (235, 236, 208),  # 7  board light square   #EBECD0
    (115, 149, 82),   # 8  board dark square    #739552
    (245, 246, 130),  # 9  highlight light      #F5F682
    (185, 202, 67),   # 10 highlight dark       #B9CA43
    (48, 46, 43),     # 11 site chrome          #302E2B
    (38, 37, 34),     # 12 panel                #262522
    (60, 59, 57),     # 13 panel raised         #3C3B39
    (129, 182, 76),   # 14 CTA green            #81B64C
    (163, 209, 96),   # 15 CTA green lit        #A3D160
    (232, 230, 227),  # 16 text primary         #E8E6E3
    (139, 137, 135),  # 17 text secondary       #8B8987
    (106, 150,  62),  # 18 logo pawn shade
    ( 74, 104,  44),  # 19 logo pawn shade side (light falls from the left)
    # nav icon colours, one light/dark pair each
    (198, 160, 110),  # 20 play tan
    (140, 110,  70),  # 21 play tan dark
    (240, 140,  70),  # 22 puzzle orange
    (190,  95,  40),  # 23 puzzle orange dark
    ( 95, 170, 225),  # 24 learn blue
    ( 55, 120, 170),  # 25 learn blue dark
    (150, 125, 215),  # 26 watch purple
    (100,  80, 160),  # 27 watch purple dark
    ( 85, 195, 165),  # 28 community teal
    ( 45, 140, 115),  # 29 community teal dark
    (215, 215, 212),  # 30 calendar paper
    (120, 118, 115),  # 31 calendar ink
    ( 86, 128,  45),  # 32 puzzle green dark
    (245, 240, 232),  # 33 banner paper
    ( 24,  22,  20),  # 34 banner ink
    (196,  36,  36),  # 35 banner red
    (250, 205,  70),  # 36 banner gold
    (250, 190,  70),  # 37 flame core
    (230, 110,  40),  # 38 flame edge
]
(TRANSPARENT, W_FILL, W_SHADE, W_LINE, B_FILL, B_SHADE, B_LINE,
 SQ_LIGHT, SQ_DARK, HL_LIGHT, HL_DARK,
 CHROME, PANEL, PANEL_UP, CTA, CTA_LIT, TEXT, TEXT_DIM,
 LOGO_SHADE, LOGO_LINE,
 PLAY_L, PLAY_D, PUZ_L, PUZ_D, LEARN_L, LEARN_D,
 WATCH_L, WATCH_D, COMM_L, COMM_D, CAL_L, CAL_D, PUZG_D,
 BAN_PAPER, BAN_INK, BAN_RED, BAN_GOLD, FLAME_L, FLAME_D) = range(39)

WHITE_INKS = (W_FILL, W_SHADE, W_LINE)
BLACK_INKS = (B_FILL, B_SHADE, B_LINE)
# chess.com's mark is a green pawn; ours is the same pawn in the CTA green
LOGO_INKS  = (CTA, LOGO_SHADE, LOGO_LINE)

SUPERSAMPLE = 6

# Every piece is authored in a 100x100 box, y pointing down, sitting on a
# baseline at y=96. chess.com's knight faces left; so does ours.
#
# Each entry is a list of primitives:
#   ("poly", [(x, y), ...])
#   ("rect", x0, y0, x1, y1, radius)
#   ("circle", cx, cy, r)
# All primitives union into one silhouette before the outline is derived, which
# is what gives Neo its single-piece-of-plastic look.
PIECES = {
    # Neo's grammar, read off the set: a short body, a collar ring, a skirt
    # that flares, and every piece standing on the same flat base slab. The
    # mass sits low - these are not tall thin Staunton silhouettes.
    # the original pawn read best: a big head, a real collar, a stepped foot
    "pawn": [
        ("circle", 50, 29, 16),
        ("poly", [(42, 41), (58, 41), (64, 64), (36, 64)]),
        ("rect", 31, 58, 69, 69, 5),
        ("poly", [(33, 69), (67, 69), (75, 81), (25, 81)]),
        ("rect", 19, 79, 81, 94, 5),
    ],
    "rook": [
        ("rect", 21, 13, 79, 32, 2),
        ("rect", 26, 30, 74, 41, 2),
        ("poly", [(32, 41), (68, 41), (71, 74), (29, 74)]),
        ("rect", 21, 71, 79, 90, 3),
    ],
    "bishop": [
        ("circle", 50, 9, 6),
        ("poly", [(43, 12), (57, 12), (69, 44), (31, 44)]),
        ("circle", 50, 38, 17),
        ("poly", [(38, 52), (62, 52), (72, 78), (28, 78)]),
        ("rect", 21, 75, 79, 92, 4),
    ],
    # The reference is simpler than everything tried before it: triangular
    # prongs fanning from a common origin, a ball rounding each tip, and one
    # wide footer overlapping the fan - wider than the fan itself. No collar,
    # no skirt.
    "queen": [
        # computed radial fan: ball r10 over prongs 10 wide at the tip, inner
        # pair 14 degrees off vertical so their balls close but do not merge
        ("circle", 15, 38, 10),
        ("circle", 35, 20, 10),
        ("circle", 65, 20, 10),
        ("circle", 85, 38, 10),
        ("poly", [(11, 41), (18, 35), (53, 70), (40, 81)]),
        ("poly", [(30, 21), (40, 19), (57, 72), (40, 76)]),
        ("poly", [(60, 19), (70, 21), (60, 76), (43, 72)]),
        ("poly", [(82, 35), (89, 41), (60, 81), (47, 70)]),
        ("rect", 12, 76, 88, 95, 8),
    ],
    # A thick cross planted on one hard mass: a barely-tapered block down to
    # the collar, then the skirt. The arch is punched through the block.
    "king": [
        ("rect", 44, 2, 56, 27, 1),
        ("rect", 33, 9, 67, 20, 1),
        ("poly", [(31, 27), (69, 27), (72, 62), (28, 62)]),
        ("rect", 29, 60, 71, 70, 2),
        ("poly", [(36, 70), (64, 70), (74, 83), (26, 83)]),
        ("rect", 19, 80, 81, 95, 4),
    ],
    # Traced clockwise from the back ear: mane down the right, then onto the
    # same slab the rest of the set stands on.
    "knight": [
        ("poly", [
            (52, 5), (60, 19), (70, 29), (76, 43), (73, 53), (79, 64),
            (80, 74), (25, 74), (28, 64), (26, 52),
            (18, 48), (10, 49), (6, 42), (9, 34), (18, 29), (27, 23),
            (33, 13), (38, 5), (44, 15),
        ]),
        ("poly", [(30, 66), (70, 66), (74, 78), (26, 78)]),
        ("rect", 21, 75, 79, 91, 4),
    ],
}

# Cut back out of the silhouette after the union.
CUTOUTS = {
    "rook": [
        ("rect", 36, 10, 45, 28, 0),
        ("rect", 55, 10, 64, 28, 0),
    ],
    # the arch through the king's crown
    "king": [
        ("rect", 42, 33, 58, 55, 8),
    ],
    "bishop": [
        # the mitre's diagonal slit: short, upper right down to lower left
        ("poly", [(57, 23), (61, 27), (47, 41), (43, 37)]),
    ],
}

# Punched in outline colour rather than cut to transparent.
DOTS = {
    "knight": [("circle", 27, 30, 5), ("circle", 12, 41, 3)],  # eye, nostril
}


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


def _silhouette(name, size, margin):
    """Render the piece mask at SUPERSAMPLE and threshold it back down."""
    big = size * SUPERSAMPLE
    scale = (size - 2 * margin) * SUPERSAMPLE / 100.0
    offset = margin * SUPERSAMPLE

    mask = Image.new("L", (big, big), 0)
    d = ImageDraw.Draw(mask)
    _draw_primitives(d, PIECES[name], scale, offset, 255)
    if name in CUTOUTS:
        _draw_primitives(d, CUTOUTS[name], scale, offset, 0)

    small = mask.resize((size, size), Image.BOX)
    return small.point(lambda v: 255 if v >= 128 else 0)


def _dot_mask(name, size, margin):
    if name not in DOTS:
        return None
    big = size * SUPERSAMPLE
    scale = (size - 2 * margin) * SUPERSAMPLE / 100.0
    offset = margin * SUPERSAMPLE
    mask = Image.new("L", (big, big), 0)
    _draw_primitives(ImageDraw.Draw(mask), DOTS[name], scale, offset, 255)
    return mask.resize((size, size), Image.BOX).point(lambda v: 255 if v >= 128 else 0)


def render_piece(name, inks, size, margin=1, directional=False):
    """One indexed frame: flat fill, 1px inner outline, shade band on the base.

    With directional=True the outline stops being uniform: edges that face left
    catch the light and take the lit ink, everything else takes the shade. That
    is what stops the small logo pawn reading as a sticker.
    """
    fill_ink, shade_ink, line_ink = inks
    sil = _silhouette(name, size, margin)
    # MinFilter erodes: the ring that erosion removes becomes the outline
    inner = sil.filter(ImageFilter.MinFilter(3))

    img = Image.new("P", (size, size), TRANSPARENT)
    img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))

    sil_px, inner_px, out_px = sil.load(), inner.load(), img.load()
    # base shade: the bottom rows of the plinth read as one darker plane
    shade_from = size - max(2, size // 12)
    for y in range(size):
        for x in range(size):
            if not sil_px[x, y]:
                continue
            if not inner_px[x, y]:
                # a left-facing edge is one with nothing filled to its left
                lit = directional and (x == 0 or not sil_px[x - 1, y])
                out_px[x, y] = fill_ink if lit else line_ink
            elif y >= shade_from:
                out_px[x, y] = shade_ink
            else:
                out_px[x, y] = fill_ink

    dots = _dot_mask(name, size, margin)
    if dots is not None:
        dot_px = dots.load()
        for y in range(size):
            for x in range(size):
                if dot_px[x, y] and sil_px[x, y]:
                    out_px[x, y] = line_ink
    return img


PIECE_ORDER = ["pawn", "knight", "bishop", "rook", "queen", "king"]


def make_pieces(size):
    frames = []
    for inks in (WHITE_INKS, BLACK_INKS):
        for name in PIECE_ORDER:
            frames.append(render_piece(name, inks, size))
    return frames


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


def write_preview(frames, size, path, zoom=6):
    """Contact sheet on a real board background, at 1x and zoomed."""
    cols, rows = 6, 2
    sheet = Image.new("RGB", (cols * size, rows * size))
    for i, frame in enumerate(frames):
        cx, cy = i % cols, i // cols
        square = Image.new("RGB", (size, size),
                           PALETTE[SQ_LIGHT] if (cx + cy) % 2 == 0 else PALETTE[SQ_DARK])
        square.paste(frame.convert("RGB"), (0, 0), frame.point(lambda v: 0 if v == 0 else 255, "1"))
        sheet.paste(square, (cx * size, cy * size))

    big = sheet.resize((sheet.width * zoom, sheet.height * zoom), Image.NEAREST)
    out = Image.new("RGB", (big.width, big.height + sheet.height + 8), PALETTE[CHROME])
    out.paste(big, (0, 0))
    out.paste(sheet, (0, big.height + 8))
    out.save(path)
    print(f"wrote {path} (preview, {zoom}x + 1x)")


def make_logo():
    """The site mark: chess.com's green pawn, at two sizes."""
    return [render_piece("pawn", LOGO_INKS, 22, directional=True),
            render_piece("pawn", LOGO_INKS, 17, directional=True)]


# Nav and panel icons, each a list of (primitives, palette index) layers painted
# in order. Same 100x100 authoring box as the pieces.
def _jigsaw(knob_ink, body_ink):
    return [
        ([("rect", 16, 16, 84, 84, 10),
          ("circle", 50, 16, 13),
          ("circle", 84, 50, 13)], body_ink),
        ([("circle", 50, 16, 7), ("circle", 84, 50, 7)], knob_ink),
    ]


ICONS = {
    # chess.com's Play mark: a hand coming down onto a pawn to move it. At 14px
    # the hand has to be a mitten - fingers do not survive the threshold - so it
    # reads as a cuff, a palm and one thumb hooked over the pawn's head.
    "play": [
        # the pawn, sunk low so the hand has room above it
        ([("circle", 38, 56, 15),
          ("poly", [(24, 96), (30, 76), (46, 76), (52, 96)]),
          ("rect", 20, 92, 56, 100, 2)], PLAY_L),
        # the hand: forearm cuff, palm, thumb reaching down the pawn's left
        ([("rect", 58, 4, 78, 30, 4),
          ("rect", 40, 22, 84, 50, 8),
          ("rect", 30, 34, 50, 46, 5)], PLAY_D),
    ],
    "puzzles": _jigsaw(PUZ_D, PUZ_L),
    "learn": [
        ([("poly", [(50, 18), (96, 42), (50, 66), (4, 42)])], LEARN_L),
        ([("poly", [(28, 52), (72, 52), (72, 80), (28, 80)])], LEARN_D),
        ([("rect", 84, 44, 90, 76, 3)], LEARN_D),
    ],
    # binoculars turn to mush at 14px; a screen with a play triangle does not
    "watch": [
        ([("rect", 6, 24, 94, 86, 12)], WATCH_L),
        ([("poly", [(40, 40), (40, 70), (68, 55)])], WATCH_D),
    ],
    "community": [
        ([("circle", 66, 38, 13), ("rect", 50, 56, 88, 84, 12)], COMM_D),
        ([("circle", 32, 32, 16), ("rect", 12, 52, 56, 84, 14)], COMM_L),
    ],
    "calendar": [
        ([("rect", 8, 18, 92, 92, 8)], CAL_L),
        ([("rect", 8, 18, 92, 38, 8), ("rect", 24, 6, 34, 26, 3),
          ("rect", 66, 6, 76, 26, 3)], CAL_D),
        ([("rect", c, r, c + 12, r + 12, 2)
          for r in (48, 68) for c in (20, 44, 68)], CAL_D),
    ],
    "puzzlemark": _jigsaw(PUZG_D, CTA),
    # the streak marker: an outer flame with a hotter core
    "flame": [
        ([("poly", [(50, 2), (74, 36), (80, 62), (68, 88), (50, 98),
                    (32, 88), (20, 62), (26, 36)])], FLAME_D),
        ([("poly", [(50, 38), (66, 64), (60, 86), (40, 86), (34, 64)])], FLAME_L),
    ],
}


def render_icon(name, size, margin=1):
    img = Image.new("P", (size, size), TRANSPARENT)
    img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
    out = img.load()

    big = size * SUPERSAMPLE
    scale = (size - 2 * margin) * SUPERSAMPLE / 100.0
    offset = margin * SUPERSAMPLE
    for prims, ink in ICONS[name]:
        mask = Image.new("L", (big, big), 0)
        _draw_primitives(ImageDraw.Draw(mask), prims, scale, offset, 255)
        small = mask.resize((size, size), Image.BOX).point(lambda v: 255 if v >= 128 else 0)
        px = small.load()
        for y in range(size):
            for x in range(size):
                if px[x, y]:
                    out[x, y] = ink
    return img


ICON_ORDER = ["play", "puzzles", "learn", "watch", "community", "calendar",
              "puzzlemark", "flame"]


def make_icons(size=14):
    return [render_icon(name, size) for name in ICON_ORDER]


def _banner_font(size):
    """Any bold system face will do - these are 468x60 ad banners at heart."""
    from PIL import ImageFont
    for path in (
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/System/Library/Fonts/Supplemental/Impact.ttf",
        "/Library/Fonts/Arial Bold.ttf",
    ):
        if Path(path).exists():
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


def _banner(width, height, bg, rule_ink, lines):
    """One ad: a ruled box with centred lines. lines is [(text, size, ink)]."""
    img = Image.new("P", (width, height), bg)
    img.putpalette([v for rgb in PALETTE for v in rgb] + [0] * (768 - 3 * len(PALETTE)))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, width - 1, height - 1], outline=rule_ink)
    d.rectangle([2, 2, width - 3, height - 3], outline=rule_ink)

    total = sum(size + 1 for _, size, _ in lines) - 1
    y = (height - total) // 2 - 1
    for text, size, ink in lines:
        font = _banner_font(size)
        w = d.textlength(text, font=font)
        d.text(((width - w) / 2, y), text, font=font, fill=ink)
        y += size + 1
    return img


def make_banners(width=272, height=30):
    """The rotating ad slot under the board. One house ad, two paid."""
    return [
        _banner(width, height, BAN_PAPER, BAN_INK, [
            ("BOBBY RAY'S GUNS 'N' THINGS", 11, BAN_RED),
            ("SHIP IT TODAY - SHOOT IT TONIGHT", 9, BAN_INK),
        ]),
        _banner(width, height, BAN_INK, BAN_GOLD, [
            ("* GOLD CROWN MEMBERSHIP *", 11, BAN_GOLD),
            ("COMING SOON - DO NOT ASK WHEN", 9, BAN_PAPER),
        ]),
        _banner(width, height, BAN_PAPER, BAN_INK, [
            ("SAN MONA MAHJONG PARLOUR", 11, BAN_INK),
            ("GAMES ARE FAIR BECAUSE MR. KLAUS SAYS SO", 8, BAN_RED),
        ]),
    ]


def main():
    big = make_pieces(34)
    small = make_pieces(20)
    write_sti(OUT_DIR / "chesspieces.sti", big)
    write_sti(OUT_DIR / "chesspiecessmall.sti", small)
    write_sti(OUT_DIR / "chesslogo.sti", make_logo())
    write_sti(OUT_DIR / "chessicons.sti", make_icons())
    write_sti(OUT_DIR / "chessbanner.sti", make_banners())
    if "--preview" in sys.argv:
        write_preview(big, 34, Path("/tmp/chess_pieces_preview.png"))
        icons = make_icons(28) + make_logo()[:1] + [render_icon("play", 28)] * 3
        write_preview(icons[:12], 28, Path("/tmp/chess_icons_preview.png"), zoom=8)


if __name__ == "__main__":
    main()
