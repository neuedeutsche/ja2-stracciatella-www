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

import os
import re

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
    (148, 146, 144),  # 37 dim white fill
    (126, 124, 121),  # 38 dim white shade
    ( 69,  66,  62),  # 39 dim white outline
    ( 55,  52,  48),  # 40 dim black fill
    ( 45,  42,  39),  # 41 dim black shade
    ( 36,  34,  31),  # 42 dim black outline
    (250, 190,  70),  # 43 flame core
    (230, 110,  40),  # 38 flame edge
]
(TRANSPARENT, W_FILL, W_SHADE, W_LINE, B_FILL, B_SHADE, B_LINE,
 SQ_LIGHT, SQ_DARK, HL_LIGHT, HL_DARK,
 CHROME, PANEL, PANEL_UP, CTA, CTA_LIT, TEXT, TEXT_DIM,
 LOGO_SHADE, LOGO_LINE,
 PLAY_L, PLAY_D, PUZ_L, PUZ_D, LEARN_L, LEARN_D,
 WATCH_L, WATCH_D, COMM_L, COMM_D, CAL_L, CAL_D, PUZG_D,
 BAN_PAPER, BAN_INK, BAN_RED, BAN_GOLD,
 DW_FILL, DW_SHADE, DW_LINE, DB_FILL, DB_SHADE, DB_LINE,
 FLAME_L, FLAME_D) = range(45)

WHITE_INKS = (W_FILL, W_SHADE, W_LINE)
BLACK_INKS = (B_FILL, B_SHADE, B_LINE)
# the same pieces pulled toward the chrome grey, blitted while the result
# card dims the board
WHITE_DIM  = (DW_FILL, DW_SHADE, DW_LINE)
BLACK_DIM  = (DB_FILL, DB_SHADE, DB_LINE)
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
    # head, then a collar bar wider than the head, then the bell: the collar
    # only reads if it overhangs both the head above and the neck below
    # Shorter than the majors on purpose, and hourglass-shaped: head, collar,
    # a bell flaring in a concave curve, and a foot lip jutting past the bell.
    "pawn": [
        ("circle", 50, 30, 15),
        # the collar is a flat bar that pokes out plainly left and right of
        # the head - a rectangle, not a taper
        ("rect", 28, 43, 72, 55, 2),
        ("poly", [(41, 49), (59, 49), (61, 57), (67, 67), (77, 78), (80, 88),
                  (20, 88), (23, 78), (33, 67), (39, 57)]),
        ("rect", 18, 85, 82, 95, 6),
    ],
    # Every edge on an exact pixel boundary (multiples of 3.125): fractional
    # coverage rounds left and right edges differently and reads as a lopsided
    # piece. Sub-pixel corner radii dropped for the same reason.
    "rook": [
        ("rect", 18.75, 12.5, 81.25, 37.5, 0),
        ("rect", 25, 28.125, 75, 40.625, 0),
        ("poly", [(28.125, 40.625), (71.875, 40.625), (75.5, 80), (24.5, 80)]),
        ("rect", 15.625, 72, 84.375, 95, 8),
    ],
    # Constructed, not organic: the body is an elongated octagon - straight
    # segments only - with a ball on top and the slot cut into the upper
    # right. Circles are reserved for balls and heads across the whole set.
    "bishop": [
        ("poly", [(38, 22), (62, 22), (75, 41), (75, 61), (62, 85),
                  (38, 85), (25, 61), (25, 41)]),
        ("rect", 19, 78, 81, 95, 8),
    ],
    "queen": [
        # the whole crown is squeezed 10% toward the centreline - a few
        # pixels skinnier at cell size without touching the proportions
        # Four copies of one sharp triangle - wide base, apex high - set side
        # by side with their bases overlapping, the outer pair leaning
        # outward. Balls round the apexes, a footer wider than the fan below.
        # the sides run diagonally inward: the crown is widest at the outer
        # balls and slightly narrower where it meets the footer
        # each spike is truncated: a short flat edge under the ball keeps the
        # tip broad instead of converging to a point
        # tips biased outward: the outer edge carries the breadth while the
        # valley-facing edge keeps its original steep slope, so the notches
        # between spikes stay crisp
        # the outer slits lean: both walls of each valley step inward at the
        # base, so the notches converge toward the centre instead of falling
        # straight down
        ("poly", [(36, 82), (55.4, 82), (41.5625, 15.625), (33.125, 15.625)]),
        ("poly", [(44.6, 82), (64, 82), (66.875, 15.625), (58.4375, 15.625)]),
        ("poly", [(28.4, 84), (52, 78), (17.78, 30.9), (9.86, 35.3)]),
        ("poly", [(48, 78), (71.6, 84), (90.14, 35.3), (82.22, 30.9)]),
        ("circle", 34, 14, 11),
        ("circle", 66, 14, 11),
        # nudged onto the same subpixel phase as the inner pair, so all four
        # balls rasterise to the same diameter at cell size
        ("circle", 13.4, 31.65, 11),
        ("circle", 86.6, 31.65, 11),
        ("rect", 15.8, 78, 84.2, 95, 8),
    ],
    # An even cross over a crown: straight column up the middle, two bows
    # arching outward to the sides. Built solid, then the teardrop hollows
    # between column and bows are cut out.
    "king": [
        ("rect", 43.5187, 3, 56.4813, 36, 0),
        ("rect", 37.0375, 9.375, 62.9625, 18.75, 0),
        # widest through the lobes, tapering as it falls so it sits back from
        # the base instead of thickening into it
        ("poly", [(45.852, 30), (37.556, 24), (27.186, 20), (15.779, 26),
                  (8.52, 38), (9.557, 54), (16.816, 68), (29.26, 78),
                  (70.74, 78), (83.184, 68), (90.443, 54), (91.48, 38),
                  (84.221, 26), (72.814, 20), (62.444, 24), (54.148, 30)]),
        ("rect", 18.89, 78, 81.11, 95, 8),
    ],
    # The knight is composed, not traced: a neck trapezoid, a head dome, a
    # muzzle bar and an ear - then one big circle carved out at the throat,
    # which is the cut that makes it read as a horse.
    "knight": [
        # One continuous mass, twelve long segments: up the back to the crest,
        # over the ear, down the brow to a drooping muzzle, jaw tucking back
        # in, chest falling to the base. The composed variants kept falling
        # apart; this one holds.
        ("poly", [
            (76, 88), (74, 48), (68, 22), (58, 6), (50, 19), (42, 9),
            (24, 34), (10, 50), (14, 60), (32, 58), (38, 70), (36, 88),
        ]),
        # the belly: a semicircle low in front, bulging past the chest line.
        # Everything is shaved flat at the foot line and the queen's foot bar
        # is overlaid after the cut.
        ("circle", 46, 88, 26),
    ],
}

# Cut back out of the silhouette after the union.
CUTOUTS = {
    "rook": [
        # each dip a pixel narrower and a pixel shallower, centres unchanged
        # spaced a pixel further apart so the middle merlon keeps its bulk
        ("rect", 32.8125, 9.375, 39.0625, 25, 0),
        ("rect", 60.9375, 9.375, 67.1875, 25, 0),
    ],
    # the teardrop hollows between the column and each bow, tails pointing
    # up toward the base of the cross
    "king": [
        # round end up and outward, points converging down toward the centre.
        # Kept tight: the reference king is chunky, so the hollows stay small
        # and the walls around them thick.
        # inner edges dead vertical at x38/x62, circles exactly tangent to
        # them: the spine between the hollows stays parallel-sided instead of
        # flaring where the circles curve away
        ("circle", 34.56, 47.6, 5.42),
        ("poly", [(29.8, 49.9), (40, 46.5), (40, 63.45)]),
        ("circle", 65.44, 47.6, 5.42),
        ("poly", [(60, 46.5), (70.2, 49.9), (60, 63.45)]),
    ],
    "bishop": [
        # the slot: cut from above the egg straight down into it
        ("rect", 52, 3, 60, 52, 2),
    ],
    # the floor: body and belly are shaved flat at the foot's top line; the
    # foot itself is an overlay, so it survives this cut
    "knight": [
        ("rect", 0, 76, 100, 112, 0),
    ],
}

# Drawn after the cutouts, so a cut can never slice into them: the bishop's
# knob rides over the slot rather than being notched by it.
OVERLAYS = {
    "bishop": [("circle", 50, 16, 10)],
    # the same foot bar the queen stands on
    "knight": [("rect", 18, 76, 82, 95, 8)],
}

# Bilaterally symmetric pieces are mirrored mechanically: the left half is
# rendered and reflected, so no threshold tie-break can ever differ between
# the sides. The bishop and knight are asymmetric by design and stay out.
MIRRORED = {"pawn", "rook", "queen", "king"}

# Punched in outline colour rather than cut to transparent.
DOTS = {
    # currently empty: the knight's eye was tried and cut - at 34px it read
    # as a smudge, and the silhouette carries the horse alone
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


# --- SVG overrides ---------------------------------------------------------
# Drop an svg/<piece>.svg beside this script and it replaces that piece's
# primitives entirely. Authoring contract (Figma "Plain SVG" export works):
#   - any square canvas / viewBox; coordinates are normalised to the 100 box
#   - filled shapes are the body; overlaps are fine, no union needed
#   - shapes filled pure red (#FF0000) are cutouts, drawn over the body
#   - strokes and transforms are NOT read: outline strokes and flatten first
#   - path commands M L H V C Q Z (absolute or relative) are supported
SVG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "svg")


def _flatten_path(d_attr):
    """One path's d attribute -> list of point-list subpaths."""
    tok = re.findall(r"[A-Za-z]|-?\d*\.?\d+(?:[eE][-+]?\d+)?", d_attr)
    i = 0
    cur = (0.0, 0.0)
    subs, pts, cmd = [], [], None

    def num():
        nonlocal i
        v = float(tok[i]); i += 1
        return v

    def bez(p0, ctrl, steps):
        out = []
        n = len(ctrl)
        for k in range(1, steps + 1):
            t = k / steps
            ps = [p0] + ctrl
            while len(ps) > 1:
                ps = [((1 - t) * a[0] + t * b[0], (1 - t) * a[1] + t * b[1])
                      for a, b in zip(ps, ps[1:])]
            out.append(ps[0])
        return out

    while i < len(tok):
        if tok[i].isalpha():
            cmd = tok[i]; i += 1
            if cmd in "Zz":
                if len(pts) > 2: subs.append(pts)
                pts = []
                continue
        rel = cmd.islower()
        c = cmd.upper()
        ox, oy = cur if rel else (0.0, 0.0)
        if c == "M":
            cur = (num() + ox, num() + oy)
            if len(pts) > 2: subs.append(pts)
            pts = [cur]
            cmd = "l" if rel else "L"  # implicit lineto after moveto
        elif c == "L":
            cur = (num() + ox, num() + oy); pts.append(cur)
        elif c == "H":
            cur = (num() + ox, cur[1]); pts.append(cur)
        elif c == "V":
            cur = (cur[0], num() + oy); pts.append(cur)
        elif c == "C":
            c1 = (num() + ox, num() + oy); c2 = (num() + ox, num() + oy)
            end = (num() + ox, num() + oy)
            pts.extend(bez(cur, [c1, c2, end], 16)); cur = end
        elif c == "Q":
            c1 = (num() + ox, num() + oy); end = (num() + ox, num() + oy)
            pts.extend(bez(cur, [c1, end], 12)); cur = end
        else:
            raise ValueError(f"unsupported SVG path command {cmd!r} - "
                             "outline strokes and flatten before exporting")
    if len(pts) > 2: subs.append(pts)
    return subs


def _load_svg(path):
    """-> (body_polys, cut_polys), each in the 0..100 box."""
    text = open(path).read()
    m = re.search(r'viewBox="([-\d.]+)[ ,]+([-\d.]+)[ ,]+([-\d.]+)[ ,]+([-\d.]+)"', text)
    if m:
        vx, vy, vw, vh = (float(g) for g in m.groups())
    else:
        vx = vy = 0.0
        wm = re.search(r'width="([\d.]+)', text)
        vw = vh = float(wm.group(1)) if wm else 100.0
    bodies, cuts = [], []
    for tag in re.findall(r"<path[^>]*>", text):
        dm = re.search(r'\bd="([^"]+)"', tag)
        if not dm: continue
        fill = ""
        fm = re.search(r'fill="([^"]+)"', tag) or re.search(r"fill:\s*([^;\"]+)", tag)
        if fm: fill = fm.group(1).strip().lower()
        is_cut = fill in ("#ff0000", "#f00", "red", "rgb(255,0,0)")
        for sub in _flatten_path(dm.group(1)):
            poly = [((x - vx) * 100.0 / vw, (y - vy) * 100.0 / vh) for x, y in sub]
            (cuts if is_cut else bodies).append(poly)
    return bodies, cuts


def _silhouette(name, size, margin):
    """Render the piece mask at SUPERSAMPLE and threshold it back down."""
    big = size * SUPERSAMPLE
    scale = (size - 2 * margin) * SUPERSAMPLE / 100.0
    offset = margin * SUPERSAMPLE

    mask = Image.new("L", (big, big), 0)
    d = ImageDraw.Draw(mask)
    svg = os.path.join(SVG_DIR, name + ".svg")
    from_svg = os.path.exists(svg)
    if from_svg:
        bodies, cuts = _load_svg(svg)
        for poly in bodies:
            d.polygon([(offset + x * scale, offset + y * scale) for x, y in poly], fill=255)
        for poly in cuts:
            d.polygon([(offset + x * scale, offset + y * scale) for x, y in poly], fill=0)
    else:
        _draw_primitives(d, PIECES[name], scale, offset, 255)
        if name in CUTOUTS:
            _draw_primitives(d, CUTOUTS[name], scale, offset, 0)
        if name in OVERLAYS:
            _draw_primitives(d, OVERLAYS[name], scale, offset, 255)

    small = mask.resize((size, size), Image.BOX)
    small = small.point(lambda v: 255 if v >= 128 else 0)
    if name in MIRRORED and not from_svg:
        from PIL import ImageOps
        half = small.crop((0, 0, size // 2, size))
        small.paste(ImageOps.mirror(half), (size - size // 2, 0))
    return small


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
    """Frames 0-11 are the live set, 12-23 the dimmed twins for the modal."""
    frames = []
    for inks in (WHITE_INKS, BLACK_INKS, WHITE_DIM, BLACK_DIM):
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
