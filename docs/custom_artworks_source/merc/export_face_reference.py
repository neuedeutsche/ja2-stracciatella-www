#!/usr/bin/env python3
"""Export every face the shipped game has, as PNGs you can open and study.

Reads Faces.slf and Binarydata.slf straight out of the installed game. Nothing
in the game directory is written to or modified.

A JA2 portrait is one base image plus small patches that the engine blits over
the eyes and mouth (see Faces.cc):

    frame 0  the whole face, eyes open, mouth shut
    frame 1  eyes, lid down          |  drawn at the eye anchor
    frame 2  eyes, lid half up       |
    frame 3  eyes, brows down        |
    frame 4  eyes, brows up          |
    frame 5  mouth, open shape A     |  drawn at the mouth anchor
    frame 6  mouth, open shape B     |
    frame 7  mouth, open shape C     |

The anchors live in prof.dat, which is encrypted, so this script recovers them
instead: it slides each patch across the base frame and keeps the position of
best match. The surroundings of an eye patch are identical to the base - only
the eye itself differs - so the minimum is sharp and unambiguous.

Output (default ~/Desktop/ja2-face-reference):

    bigfaces/        106x122, the A.I.M. hiring portrait
    talking/         the 48x43 face, all 8 frames, transparent PNGs
    talking_sheets/  per merc: the frames laid out, and each patch composited
                     onto the base where the engine will put it
    65face/ 33face/  the two small sizes, 8 frames each
    npc_faces/       the bNN heads and their a-e expression variants
    palettes/        each sheet's 256 colours as a grid
    measured_offsets.csv
    bios.txt         the A.I.M. bios, decoded

Usage:  python3 export_face_reference.py [--out DIR] [--game-dir DIR] [--only 7,17]
"""

import argparse
import csv
import json
import os
import struct
from pathlib import Path

from PIL import Image

from ja2faces import SlfArchive, StiImage, read_jsonc

REPO = Path(__file__).resolve().parents[3]
PROFILE_INFO = REPO / "assets" / "externalized" / "mercs-profile-info.json"

# the four sizes the engine loads, by the subdirectory it looks in
FACE_SETS = [
	("bigfaces", "BIGFACES\\{:02d}.STI"),
	("talking",  "{:02d}.STI"),
	("65face",   "65FACE\\{:02d}.STI"),
	("33face",   "33FACE\\{:02d}.STI"),
]

EYE_FRAMES = (1, 2, 3, 4)
MOUTH_FRAMES = (5, 6, 7)
FRAME_LABELS = {
	0: "base",
	1: "eyes_lid_down",
	2: "eyes_lid_half",
	3: "eyes_brows_down",
	4: "eyes_brows_up",
	5: "mouth_a",
	6: "mouth_b",
	7: "mouth_c",
}


def game_dir(explicit=None):
	if explicit:
		return Path(explicit)
	cfg = Path.home() / ".ja2" / "ja2.json"
	if cfg.exists():
		try:
			data = read_jsonc(cfg)
			if data.get("game_dir"):
				return Path(data["game_dir"])
		except Exception:
			pass
	raise SystemExit("could not find the game directory; pass --game-dir")


def profile_names():
	"""profile id -> internal name, so the files are readable."""
	names = {}
	for entry in read_jsonc(PROFILE_INFO):
		names[entry["profileID"]] = entry.get("internalName", "")
	return names


def decode_edt(blob, record_size):
	"""EDT text: UTF-16LE with every code unit shifted up by one, except the
	space, which is stored as itself."""
	out = []
	for start in range(0, len(blob), record_size):
		units = struct.unpack_from("<%dH" % (record_size // 2), blob, start)
		chars = []
		for u in units:
			if u == 0:
				continue
			chars.append(" " if u == 0x20 else chr((u - 1) % 0x110000))
		out.append("".join(chars).strip())
	return out


def best_offset(base, patch):
	"""Where this patch sits on the base frame: the position of least
	difference. Returns (x, y, score, margin) - margin is how much better the
	winner is than the runner-up, so a low margin means don't trust it."""
	import numpy as np

	b = np.asarray(base.convert("RGB"), dtype=np.int32)
	p = np.asarray(patch.convert("RGB"), dtype=np.int32)
	mask = np.asarray(patch.split()[-1], dtype=np.int32) > 0
	ph, pw = p.shape[:2]
	bh, bw = b.shape[:2]
	if ph > bh or pw > bw or not mask.any():
		return None

	scores = []
	for y in range(bh - ph + 1):
		for x in range(bw - pw + 1):
			window = b[y:y + ph, x:x + pw]
			diff = np.abs(window - p).sum(axis=2)
			scores.append((float(diff[mask].mean()), x, y))
	scores.sort()
	best = scores[0]
	runner = next((s for s in scores[1:] if abs(s[1] - best[1]) > 1 or abs(s[2] - best[2]) > 1), None)
	margin = (runner[0] - best[0]) if runner else 0.0
	return best[1], best[2], best[0], margin


def contact_sheet(sti, eye_at, mouth_at, scale=4):
	"""The base, every frame, and every patch composited where it belongs."""
	base = sti.to_image(0)
	bw, bh = base.size

	tiles = []
	tiles.append(("base", base))
	for frame in range(1, sti.count):
		patch = sti.to_image(frame)
		# the patch alone, on a slate so its extent is visible
		alone = Image.new("RGBA", (bw, bh), (30, 30, 34, 255))
		anchor = eye_at if frame in EYE_FRAMES else mouth_at
		if anchor:
			alone.paste(patch, anchor, patch)
		tiles.append((FRAME_LABELS.get(frame, str(frame)) + " (placed)", alone))

		composed = base.copy()
		if anchor:
			composed.paste(patch, anchor, patch)
		tiles.append((FRAME_LABELS.get(frame, str(frame)), composed))

	cols = 5
	rows = (len(tiles) + cols - 1) // cols
	pad = 6
	cw, ch = bw * scale + pad, bh * scale + pad + 10
	sheet = Image.new("RGB", (cols * cw + pad, rows * ch + pad), (18, 18, 20))
	for i, (label, img) in enumerate(tiles):
		big = img.convert("RGB").resize((bw * scale, bh * scale), Image.NEAREST)
		x = pad + (i % cols) * cw
		y = pad + (i // cols) * ch
		sheet.paste(big, (x, y))
	return sheet


def export_merc(faces, pid, name, out, want_sheets=True):
	"""Everything for one profile. Returns the measured-offset row, or None."""
	row = None
	for folder, pattern in FACE_SETS:
		key = pattern.format(pid)
		if key not in faces:
			continue
		sti = StiImage(faces.read(key))
		stem = "{:03d}_{}".format(pid, name or "unnamed")

		dest = out / folder
		dest.mkdir(parents=True, exist_ok=True)

		if sti.count == 1:
			sti.to_image(0).save(dest / (stem + ".png"))
			continue

		for frame in range(sti.count):
			label = FRAME_LABELS.get(frame, "f%d" % frame)
			sti.to_image(frame).save(dest / "{}_{}_{}.png".format(stem, frame, label))

		base = sti.to_image(0)
		eye = best_offset(base, sti.to_image(1)) if sti.count > 1 else None
		mouth = best_offset(base, sti.to_image(5)) if sti.count > 5 else None
		eye_at = (eye[0], eye[1]) if eye else None
		mouth_at = (mouth[0], mouth[1]) if mouth else None

		if folder == "talking":
			ew, eh = sti.sub_size(1)
			mw, mh = sti.sub_size(5)
			row = {
				"profile": pid, "name": name,
				"base_w": base.size[0], "base_h": base.size[1],
				"eyes_x": eye_at[0] if eye_at else "", "eyes_y": eye_at[1] if eye_at else "",
				"eyes_w": ew, "eyes_h": eh,
				"eyes_confidence": round(eye[3], 2) if eye else "",
				"mouth_x": mouth_at[0] if mouth_at else "", "mouth_y": mouth_at[1] if mouth_at else "",
				"mouth_w": mw, "mouth_h": mh,
				"mouth_confidence": round(mouth[3], 2) if mouth else "",
			}

		if want_sheets:
			sheets = out / (folder + "_sheets")
			sheets.mkdir(parents=True, exist_ok=True)
			contact_sheet(sti, eye_at, mouth_at).save(sheets / (stem + ".png"))

		pal = out / "palettes"
		pal.mkdir(parents=True, exist_ok=True)
		sti.palette_strip().save(pal / (stem + "_" + folder + ".png"))
	return row


def export_npc_faces(faces, names, out):
	"""The bNN heads, and the a-e variants some NPCs carry."""
	dest = out / "npc_faces"
	dest.mkdir(parents=True, exist_ok=True)
	written = 0
	for key in faces.names():
		head = key.split("\\")[-1]
		if not head.startswith("B") or not head.endswith(".STI"):
			continue
		stem_raw = head[1:-4]
		digits = "".join(c for c in stem_raw if c.isdigit())
		if not digits:
			continue
		suffix = stem_raw[len(digits):].lower()
		pid = int(digits)
		try:
			sti = StiImage(faces.read(key))
		except ValueError:
			continue
		stem = "{:03d}_{}{}".format(pid, names.get(pid, "unnamed"), "_" + suffix if suffix else "")
		if sti.count == 1:
			sti.to_image(0).save(dest / (stem + ".png"))
		else:
			for frame in range(sti.count):
				label = FRAME_LABELS.get(frame, "f%d" % frame)
				sti.to_image(frame).save(dest / "{}_{}_{}.png".format(stem, frame, label))
		written += 1
	return written


def overview_sheet(faces, names, pattern, out_path, cols=10, scale=2, label_h=10):
	"""Every portrait the game has, on one wall, so the house style is visible
	at a glance rather than one file at a time."""
	from PIL import ImageDraw
	found = []
	for pid in range(170):
		key = pattern.format(pid)
		if key in faces:
			try:
				found.append((pid, StiImage(faces.read(key)).to_image(0)))
			except ValueError:
				pass
	if not found:
		return 0

	tw, th = found[0][1].size
	cw, ch = tw * scale + 4, th * scale + 4 + label_h
	rows = (len(found) + cols - 1) // cols
	sheet = Image.new("RGB", (cols * cw, rows * ch), (18, 18, 20))
	draw = ImageDraw.Draw(sheet)
	for i, (pid, img) in enumerate(found):
		x = (i % cols) * cw + 2
		y = (i // cols) * ch + 2
		sheet.paste(img.convert("RGB").resize((tw * scale, th * scale), Image.NEAREST), (x, y))
		draw.text((x, y + th * scale + 1),
		          "{:03d} {}".format(pid, (names.get(pid, "") or "")[:12]),
		          fill=(150, 148, 144))
	sheet.save(out_path)
	return len(found)


def export_bios(data_dir, out):
	binary = SlfArchive(data_dir / "Binarydata.slf")
	lines = []
	if "AIMBIOS.EDT" in binary:
		for i, text in enumerate(decode_edt(binary.read("AIMBIOS.EDT"), 1120)):
			if text:
				lines.append("--- profile {:03d} ---\n{}\n".format(i, text))
	(out / "bios.txt").write_text("\n".join(lines))
	return len(lines)


def main():
	ap = argparse.ArgumentParser(description=__doc__,
	                             formatter_class=argparse.RawDescriptionHelpFormatter)
	ap.add_argument("--out", default=str(Path.home() / "Desktop" / "ja2-face-reference"))
	ap.add_argument("--game-dir", default=None)
	ap.add_argument("--only", default=None, help="comma-separated profile ids")
	ap.add_argument("--no-sheets", action="store_true")
	args = ap.parse_args()

	data_dir = game_dir(args.game_dir) / "Data"
	if not data_dir.exists():
		data_dir = game_dir(args.game_dir) / "data"
	out = Path(args.out)
	out.mkdir(parents=True, exist_ok=True)

	faces = SlfArchive(data_dir / "Faces.slf")
	names = profile_names()

	wanted = range(0, 170)
	if args.only:
		wanted = [int(x) for x in args.only.split(",")]

	rows = []
	done = 0
	for pid in wanted:
		row = export_merc(faces, pid, names.get(pid, ""), out, not args.no_sheets)
		if row:
			rows.append(row)
		if any(pattern.format(pid) in faces for _, pattern in FACE_SETS):
			done += 1
			print("  {:03d} {}".format(pid, names.get(pid, "")))

	if rows:
		with open(out / "measured_offsets.csv", "w", newline="") as f:
			writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
			writer.writeheader()
			writer.writerows(rows)

	npc = export_npc_faces(faces, names, out)
	bios = export_bios(data_dir, out)

	overview_sheet(faces, names, "BIGFACES\\{:02d}.STI", out / "overview_bigfaces.png")
	overview_sheet(faces, names, "{:02d}.STI", out / "overview_talking.png", cols=12, scale=3)

	print()
	print("exported {} profiles, {} npc face sheets, {} bios".format(done, npc, bios))
	print("into {}".format(out))
	if rows:
		import statistics
		ex = [r["eyes_x"] for r in rows if r["eyes_x"] != ""]
		ey = [r["eyes_y"] for r in rows if r["eyes_y"] != ""]
		mx = [r["mouth_x"] for r in rows if r["mouth_x"] != ""]
		my = [r["mouth_y"] for r in rows if r["mouth_y"] != ""]
		print("median anchors on the 48x43 face: eyes ({}, {})  mouth ({}, {})".format(
			int(statistics.median(ex)), int(statistics.median(ey)),
			int(statistics.median(mx)), int(statistics.median(my))))


if __name__ == "__main__":
	main()
