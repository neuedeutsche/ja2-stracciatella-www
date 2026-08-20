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
    (61, 59, 56),     # 3  white piece outline
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
]
(TRANSPARENT, W_FILL, W_SHADE, W_LINE, B_FILL, B_SHADE, B_LINE,
 SQ_LIGHT, SQ_DARK, HL_LIGHT, HL_DARK,
 CHROME, PANEL, PANEL_UP, CTA, CTA_LIT, TEXT, TEXT_DIM) = range(18)

WHITE_INKS = (W_FILL, W_SHADE, W_LINE)
BLACK_INKS = (B_FILL, B_SHADE, B_LINE)

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
    "pawn": [
        ("circle", 50, 31, 17),
        ("poly", [(41, 43), (59, 43), (66, 67), (34, 67)]),
        ("rect", 30, 61, 70, 72, 5),
        ("poly", [(31, 72), (69, 72), (77, 83), (23, 83)]),
        ("rect", 16, 81, 84, 96, 6),
    ],
    "rook": [
        # battlements: one slab, two notches cut back out after the union
        ("rect", 21, 17, 79, 35, 3),
        ("rect", 27, 33, 73, 45, 3),
        ("poly", [(34, 45), (66, 45), (64, 67), (36, 67)]),
        ("poly", [(32, 67), (68, 67), (79, 82), (21, 82)]),
        ("rect", 14, 80, 86, 96, 5),
    ],
    "bishop": [
        ("circle", 50, 12, 7),
        ("poly", [(43, 19), (57, 19), (68, 42), (32, 42)]),
        ("circle", 50, 38, 18),
        ("rect", 28, 49, 72, 60, 5),
        ("poly", [(33, 60), (67, 60), (76, 81), (24, 81)]),
        ("rect", 14, 80, 86, 96, 5),
    ],
    # crown as a solid band plus five fat wedges, balls on the tips - the
    # zigzag-polygon version reads as wire at 34px
    "queen": [
        ("circle", 50, 9, 7),
        ("circle", 31, 14, 7),
        ("circle", 69, 14, 7),
        ("circle", 13, 23, 7),
        ("circle", 87, 23, 7),
        ("poly", [(43, 46), (57, 46), (50, 12)]),
        ("poly", [(25, 46), (38, 46), (31, 17)]),
        ("poly", [(62, 46), (75, 46), (69, 17)]),
        ("poly", [(16, 46), (28, 46), (13, 26)]),
        ("poly", [(72, 46), (84, 46), (87, 26)]),
        ("rect", 17, 42, 83, 58, 2),
        ("rect", 22, 56, 78, 67, 5),
        ("poly", [(30, 67), (70, 67), (78, 82), (22, 82)]),
        ("rect", 13, 80, 87, 96, 5),
    ],
    "king": [
        ("rect", 45, 2, 55, 30, 2),
        ("rect", 34, 11, 66, 21, 2),
        ("circle", 30, 34, 9),
        ("circle", 70, 34, 9),
        ("poly", [(23, 32), (77, 32), (73, 56), (27, 56)]),
        ("rect", 22, 54, 78, 66, 5),
        ("poly", [(30, 66), (70, 66), (78, 82), (22, 82)]),
        ("rect", 13, 80, 87, 96, 5),
    ],
    # Traced clockwise from the back ear: mane down the right, plinth, up the
    # chest, out along the throat to the muzzle, then back up the face.
    "knight": [
        ("poly", [
            (52, 6), (60, 20), (70, 30), (76, 44), (73, 54), (79, 66),
            (81, 80), (81, 85), (23, 85), (23, 80), (27, 68), (25, 56),
            (18, 52), (10, 53), (6, 46), (9, 38), (18, 33), (27, 27),
            (33, 17), (38, 9), (44, 19),
        ]),
        ("rect", 14, 82, 86, 96, 5),
    ],
}

# Cut back out of the silhouette after the union.
CUTOUTS = {
    "rook": [
        ("rect", 36, 13, 45, 30, 0),
        ("rect", 55, 13, 64, 30, 0),
    ],
    "bishop": [
        # the mitre's diagonal slit: short, upper right down to lower left
        ("poly", [(57, 24), (61, 28), (48, 41), (44, 37)]),
    ],
}

# Punched in outline colour rather than cut to transparent.
DOTS = {
    "knight": [("circle", 27, 34, 5), ("circle", 13, 45, 3)],  # eye, nostril
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


def render_piece(name, inks, size, margin=1):
    """One indexed frame: flat fill, 1px inner outline, shade band on the base."""
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
                out_px[x, y] = line_ink
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


def main():
    big = make_pieces(34)
    small = make_pieces(20)
    write_sti(OUT_DIR / "chesspieces.sti", big)
    write_sti(OUT_DIR / "chesspiecessmall.sti", small)
    if "--preview" in sys.argv:
        write_preview(big, 34, Path("/tmp/chess_pieces_preview.png"))


if __name__ == "__main__":
    main()
