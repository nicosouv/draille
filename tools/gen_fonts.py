#!/usr/bin/env python3
"""
Draille — Font generator
Converts Inter TTF → TFT_eSPI .vlw smooth font format
Requires: pip install freetype-py

Usage:
  python3 tools/gen_fonts.py

Output: data/Inter_*.vlw  (ready to flash with: make fs-flash)

Download Inter from: https://fonts.google.com/specimen/Inter
"""

import os
import struct
import sys

try:
    import freetype
except ImportError:
    print("Missing dependency: pip install freetype-py")
    sys.exit(1)

FONTS = [
    ("Inter_28pt-Bold.ttf",   48, "Inter_Bold_48"),
    ("Inter_24pt-Medium.ttf", 24, "Inter_Medium_24"),
    ("Inter_18pt-Light.ttf",  18, "Inter_Light_18"),
]

# Characters to include (ASCII printable + common)
CHARSET = (
    " !\"#$%&'()*+,-./"
    "0123456789:;<=>?@"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz"
    "{|}~"
    "°·"
)

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "data")
FONT_DIR   = os.path.join(os.path.dirname(__file__), "..", "fonts")


def render_vlw(ttf_path: str, size: int, output_path: str):
    face = freetype.Face(ttf_path)
    face.set_pixel_sizes(0, size)

    glyphs = []
    for ch in CHARSET:
        cp = ord(ch)
        face.load_char(ch, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_NORMAL)
        g = face.glyph
        bm = g.bitmap
        glyphs.append({
            "codepoint": cp,
            "width":     bm.width,
            "height":    bm.rows,
            "advance":   g.advance.x >> 6,
            "x_off":     g.bitmap_left,
            "y_off":     g.bitmap_top,
            "bitmap":    bytes(bm.buffer),
        })

    with open(output_path, "wb") as f:
        # VLW header
        f.write(struct.pack(">HH", len(glyphs), size))   # count, yAdvance
        f.write(struct.pack(">H", 0))                     # spare1
        f.write(struct.pack(">H", 0))                     # spare2
        f.write(struct.pack(">H", 0))                     # spare3

        for g in glyphs:
            f.write(struct.pack(">I", g["codepoint"]))
            f.write(struct.pack(">H", g["height"]))
            f.write(struct.pack(">H", g["width"]))
            f.write(struct.pack(">H", g["advance"]))
            f.write(struct.pack(">h", g["x_off"]))
            f.write(struct.pack(">h", g["y_off"] - g["height"]))
            f.write(struct.pack(">H", 0))  # padding

        for g in glyphs:
            f.write(g["bitmap"])

    kb = os.path.getsize(output_path) / 1024
    print(f"  {os.path.basename(output_path):30s}  {kb:.1f} KB  ({len(glyphs)} glyphs)")


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print(f"\nDraille font generator")
    print(f"Font source : {FONT_DIR}")
    print(f"Output      : {OUTPUT_DIR}\n")

    all_ok = True
    for ttf_name, size, out_name in FONTS:
        ttf_path = os.path.join(FONT_DIR, ttf_name)
        if not os.path.exists(ttf_path):
            print(f"  MISSING: {ttf_path}")
            all_ok = False
            continue
        out_path = os.path.join(OUTPUT_DIR, out_name + ".vlw")
        render_vlw(ttf_path, size, out_path)

    if not all_ok:
        print(f"\n→ Download Inter at https://fonts.google.com/specimen/Inter")
        print(f"  Extract into: {FONT_DIR}/")
        print(f"  Files needed: Inter_28pt-Bold.ttf, Inter_24pt-Medium.ttf, Inter_18pt-Light.ttf\n")
        sys.exit(1)
    else:
        print(f"\n→ Run: make fs-flash  to upload fonts to the device\n")


if __name__ == "__main__":
    main()
