#!/usr/bin/env python3
"""Convert a BDF bitmap font into the kernel's font format.

The kernel uses pixel fonts: every pixel of a glyph is either on or off, so
text is perfectly sharp on the GBA screen. The sources are BDF files (a plain
text bitmap format) in assets/fonts/src/; this script packs the printable
ASCII characters into the compact "OFNT" format the kernel reads in place.

File format (little-endian), see src/gfx/font.h:

    offset  size  field
    0       4     magic "OFNT"
    4       1     line height in pixels
    5       1     ascent (baseline position from the top of a line)
    6       1     first character code (32)
    7       1     number of glyphs (95: ' ' .. '~')
    8       8*n   glyph records:
                    u16 bitmap offset (from the start of the bitmap area)
                    u8  width, u8 height
                    s8  x offset, s8 y offset (from the top of the line)
                    u8  advance, u8 reserved
    ...           bitmaps, row by row, 2 pixels per byte (first pixel in the
                  high nibble), each row padded to a whole byte

Pixels are stored as 4-bit coverage (0 = off, 15 = on) so the renderer could
also draw anti-aliased fonts; BDF fonts only ever use 0 and 15.

Usage:
    make_font.py FONT.bdf OUTPUT.bin
    make_font.py --all [--check]      regenerate (or verify) every font in
                                      assets/fonts/fonts.txt

Uses only the Python standard library.
"""
import argparse
import os
import struct
import sys

FIRST_CHAR = 32
LAST_CHAR = 126
ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
FONT_DIR = os.path.join(ROOT, 'assets', 'fonts')


def read_bdf(path):
    """Return (ascent, descent, {code: (dwidth, (w, h, xoff, yoff), rows)})."""
    ascent = descent = None
    glyphs = {}
    glyph = None
    rows = None
    with open(path, encoding='utf-8') as f:
        for line in f:
            parts = line.split()
            if not parts:
                continue
            key = parts[0]
            if key == 'FONT_ASCENT':
                ascent = int(parts[1])
            elif key == 'FONT_DESCENT':
                descent = int(parts[1])
            elif key == 'STARTCHAR':
                glyph = {}
            elif key == 'ENCODING':
                glyph['code'] = int(parts[1])
            elif key == 'DWIDTH':
                glyph['advance'] = int(parts[1])
            elif key == 'BBX':
                glyph['bbx'] = tuple(int(p) for p in parts[1:5])
            elif key == 'BITMAP':
                rows = []
            elif key == 'ENDCHAR':
                glyphs[glyph['code']] = (glyph['advance'], glyph['bbx'], rows)
                glyph = rows = None
            elif rows is not None:
                rows.append(parts[0])
    if ascent is None or descent is None:
        sys.exit(f'{path}: FONT_ASCENT/FONT_DESCENT missing')
    return ascent, descent, glyphs


def convert(path):
    ascent, descent, glyphs = read_bdf(path)
    records, bitmaps = [], bytearray()
    for code in range(FIRST_CHAR, LAST_CHAR + 1):
        if code not in glyphs:
            sys.exit(f'{path}: character {code} missing')
        advance, (w, h, xoff, yoff), rows = glyphs[code]
        offset = len(bitmaps)
        for hexrow in rows[:h]:
            bits = int(hexrow, 16)
            nbits = len(hexrow) * 4
            pixels = [15 if bits >> (nbits - 1 - x) & 1 else 0 for x in range(w)]
            if w % 2:
                pixels.append(0)
            for x in range(0, len(pixels), 2):
                bitmaps.append(pixels[x] << 4 | pixels[x + 1])
        # BDF offsets are from the baseline (y up); ours from the line top (y down).
        records.append((offset, w, h, xoff, ascent - (h + yoff), advance))

    if len(bitmaps) > 0xFFFF:
        sys.exit('font too large for 16-bit offsets')
    out = bytearray(b'OFNT')
    out += struct.pack('<BBBB', ascent + descent, ascent, FIRST_CHAR, len(records))
    for offset, w, h, xo, yo, adv in records:
        out += struct.pack('<HBBbbBB', offset, w, h, xo, yo, adv, 0)
    out += bitmaps
    return bytes(out)


def read_manifest():
    """Lines of fonts.txt: output-name source.bdf."""
    entries = []
    with open(os.path.join(FONT_DIR, 'fonts.txt')) as f:
        for line in f:
            line = line.split('#', 1)[0].split()
            if line:
                entries.append((line[0], line[1]))
    return entries


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('font', nargs='?')
    ap.add_argument('output', nargs='?')
    ap.add_argument('--all', action='store_true', help='process assets/fonts/fonts.txt')
    ap.add_argument('--check', action='store_true', help='with --all: only verify')
    args = ap.parse_args()

    if not args.all:
        if not (args.font and args.output):
            ap.error('FONT OUTPUT required (or --all)')
        with open(args.output, 'wb') as f:
            f.write(convert(args.font))
        return 0

    stale = []
    for name, source in read_manifest():
        data = convert(os.path.join(FONT_DIR, 'src', source))
        target = os.path.join(FONT_DIR, name + '.bin')
        if args.check:
            if not os.path.exists(target) or open(target, 'rb').read() != data:
                stale.append(target)
        else:
            with open(target, 'wb') as f:
                f.write(data)
            print(f'{target}: {len(data)} bytes')
    if stale:
        print('out of date (run tools/make_font.py --all):', *stale, sep='\n  ')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
