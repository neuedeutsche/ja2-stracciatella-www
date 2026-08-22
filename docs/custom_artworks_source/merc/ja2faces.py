#!/usr/bin/env python3
"""Readers for the shipped JA2 art archives: SLF libraries and STI images.

Nothing here writes to the game data - these are decoders, used by the face
reference exporter and by the template generator.

The ETRLE decoder is written from the file format rather than ported from the
engine: SGPVObject::GetETRLEPixelValue mis-walks multi-row images (it does not
step over control bytes or row terminators), which cost this project a day of
misaligned portrait patches once already.
"""

import struct
from pathlib import Path

# SLF directory entries are fixed-size records at the tail of the file
SLF_ENTRY_SIZE = 280
SLF_HEADER_SIZE = 532

STCI_TRANSPARENT = 0x0001
STCI_INDEXED     = 0x0008
STCI_ETRLE       = 0x0020


class SlfArchive:
	"""One .slf library. Keys are the stored paths, backslashed and uppercase."""

	def __init__(self, path):
		self.path = Path(path)
		self._data = self.path.read_bytes()
		count = struct.unpack_from("<i", self._data, 512)[0]
		start = len(self._data) - count * SLF_ENTRY_SIZE
		self._entries = {}
		for i in range(count):
			rec = self._data[start + i * SLF_ENTRY_SIZE:start + (i + 1) * SLF_ENTRY_SIZE]
			name = rec[:256].split(b"\0")[0].decode("latin-1")
			offset, length = struct.unpack_from("<II", rec, 256)
			self._entries[name.upper()] = (offset, length)

	def __contains__(self, name):
		return self._key(name) in self._entries

	def __iter__(self):
		return iter(sorted(self._entries))

	def names(self):
		return sorted(self._entries)

	@staticmethod
	def _key(name):
		return name.replace("/", "\\").upper()

	def read(self, name):
		offset, length = self._entries[self._key(name)]
		return self._data[offset:offset + length]


class StiImage:
	"""An indexed, ETRLE-compressed STI sheet: a palette and N sub-images."""

	def __init__(self, blob):
		if blob[:4] != b"STCI":
			raise ValueError("not an STI")
		self.flags = struct.unpack_from("<I", blob, 16)[0]
		self.sheet_h, self.sheet_w = struct.unpack_from("<HH", blob, 20)
		if not self.indexed:
			raise ValueError("only indexed STIs are supported")
		self.colour_count, self.count = struct.unpack_from("<IH", blob, 24)

		pal_at = 64
		raw_pal = blob[pal_at:pal_at + 768]
		self.palette = [tuple(raw_pal[i * 3:i * 3 + 3]) for i in range(256)]

		self._blob = blob
		self._sub = []
		base = pal_at + 768
		for i in range(self.count):
			off, length, ox, oy, h, w = struct.unpack_from("<IIhhHH", blob, base + i * 16)
			self._sub.append({"offset": off, "length": length,
			                  "x": ox, "y": oy, "w": w, "h": h})
		self._data_at = base + self.count * 16

	@property
	def indexed(self):
		return bool(self.flags & STCI_INDEXED)

	@property
	def compressed(self):
		return bool(self.flags & STCI_ETRLE)

	def sub_size(self, index):
		s = self._sub[index]
		return s["w"], s["h"]

	def sub_offset(self, index):
		s = self._sub[index]
		return s["x"], s["y"]

	def indices(self, index):
		"""Sub-image as a list of rows of palette indices, None where clear."""
		s = self._sub[index]
		w, h = s["w"], s["h"]
		data = self._blob[self._data_at + s["offset"]:self._data_at + s["offset"] + s["length"]]

		rows = []
		pos = 0
		for _ in range(h):
			row = []
			while pos < len(data):
				control = data[pos]
				pos += 1
				if control == 0:                      # end of row
					break
				if control & 0x80:                    # a run of transparent
					row.extend([None] * (control & 0x7F))
				else:                                 # literal pixels
					row.extend(data[pos:pos + control])
					pos += control
			# rows are padded or truncated to the declared width
			row = (row + [None] * w)[:w]
			rows.append(row)
		while len(rows) < h:
			rows.append([None] * w)
		return rows

	def to_image(self, index, background=None):
		"""Sub-image as an RGBA Pillow image; background fills clear pixels."""
		from PIL import Image
		w, h = self.sub_size(index)
		img = Image.new("RGBA", (w, h), background or (0, 0, 0, 0))
		px = img.load()
		for y, row in enumerate(self.indices(index)):
			for x, idx in enumerate(row):
				if idx is None:
					continue
				r, g, b = self.palette[idx]
				px[x, y] = (r, g, b, 255)
		return img

	def palette_strip(self, swatch=8):
		"""The colour table as a 16x16 grid of swatches, for eyeballing."""
		from PIL import Image
		img = Image.new("RGB", (16 * swatch, 16 * swatch))
		px = img.load()
		for i, (r, g, b) in enumerate(self.palette):
			cx, cy = (i % 16) * swatch, (i // 16) * swatch
			for y in range(swatch):
				for x in range(swatch):
					px[cx + x, cy + y] = (r, g, b)
		return img


def read_jsonc(path):
	"""The externalized data is JSON with comments; strip them, respecting
	string literals so a // inside a quoted value survives."""
	import json
	text = Path(path).read_text()
	out = []
	i, n = 0, len(text)
	in_string = False
	while i < n:
		c = text[i]
		if in_string:
			out.append(c)
			if c == "\\" and i + 1 < n:
				out.append(text[i + 1])
				i += 2
				continue
			if c == '"':
				in_string = False
			i += 1
			continue
		if c == '"':
			in_string = True
			out.append(c)
			i += 1
			continue
		if text.startswith("//", i):
			while i < n and text[i] != "\n":
				i += 1
			continue
		if text.startswith("/*", i):
			end = text.find("*/", i + 2)
			i = n if end < 0 else end + 2
			continue
		out.append(c)
		i += 1
	# tolerate trailing commas, which the game's loader also accepts
	cleaned = "".join(out)
	import re
	cleaned = re.sub(r",(\s*[}\]])", r"\1", cleaned)
	return json.loads(cleaned)
