#!/usr/bin/env python3
"""Convert BDF bitmap fonts into the kernel's font format.

The kernel uses pixel fonts: every pixel of a glyph is either on or off, so
text is sharp on the GBA screen. The sources are BDF files (a plain text
bitmap format) in assets/fonts/src/; this script packs the characters the
kernel can show (RANGES) into the compact "OFN2" format it reads in place.

File format (little-endian), see src/gfx/font.c:

    offset  size  field
    0       4     magic "OFN2"
    4       1     line height in pixels
    5       1     ascent (baseline position from the top of a line)
    6       2     number of ranges R
    8       8*R   ranges: u32 first code point, u16 count, u16 reserved
    ...     12*n  glyph records, for every code point of every range:
                    u32 bitmap offset (from the start of the bitmap area)
                    u8  width, u8 height
                    s8  x offset, s8 y offset (from the top of the line)
                    u8  advance, 3 bytes reserved
    ...           bitmaps: 1 bit per pixel, rows padded to whole bytes,
                  most significant bit = leftmost pixel

A character missing from a font is taken from its fallback font (if one is
given in fonts.txt), otherwise it is drawn as '?'.

Usage:
    make_font.py --all [--check]       regenerate (or verify) every font in
                                       assets/fonts/fonts.txt
    make_font.py --subset IN.bdf OUT.bdf
                                       copy only the RANGES characters of a
                                       full BDF font (to keep sources small)

Uses only the Python standard library.
"""
import argparse
import os
import struct
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
FONT_DIR = os.path.join(ROOT, 'assets', 'fonts')

# Characters the kernel can show, as inclusive (first, last) code points.
RANGES = [
    (0x0020, 0x007E),  # ASCII
    (0x00A0, 0x017F),  # Latin-1 Supplement, Latin Extended-A (accents)
    (0x0370, 0x03FF),  # Greek
    (0x0400, 0x045F),  # Cyrillic
    (0x2010, 0x2027),  # dashes, quotes, ellipsis
    (0x2122, 0x2122),  # trade mark
    (0x3000, 0x3003),  # CJK punctuation
    (0x3040, 0x30FF),  # Hiragana, Katakana
    (0xFF01, 0xFF5E),  # full-width ASCII
]


def wanted(code):
    return any(first <= code <= last for first, last in RANGES)


def read_bdf(path):
    """Return (ascent, descent, {code: (advance, (w, h, xoff, yoff), rows)})."""
    ascent = descent = None
    glyphs = {}
    glyph = rows = None
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


def pack_rows(rows, w, h):
    """BDF hex rows -> 1 bpp rows padded to bytes, MSB = leftmost pixel."""
    out = bytearray()
    row_bytes = (w + 7) // 8
    for hexrow in rows[:h]:
        bits = int(hexrow, 16)
        nbits = len(hexrow) * 4
        value = 0
        for x in range(row_bytes * 8):
            on = x < w and (bits >> (nbits - 1 - x)) & 1
            value = value << 1 | on
        out += value.to_bytes(row_bytes, 'big')
    return out


def convert(path, fallback_path=None):
    ascent, descent, glyphs = read_bdf(path)
    fallback = {}
    if fallback_path:
        f_ascent, f_descent, fallback = read_bdf(fallback_path)
        if (f_ascent, f_descent) != (ascent, descent):
            sys.exit(f'{fallback_path}: metrics differ from {path}')

    bitmaps = bytearray()
    placed = {}  # code -> record, so '?' can be reused

    def place(source):
        advance, (w, h, xoff, yoff), rows = source
        offset = len(bitmaps)
        bitmaps.extend(pack_rows(rows, w, h))
        # BDF offsets are from the baseline (y up); ours from the line top.
        return (offset, w, h, xoff, ascent - (h + yoff), advance)

    question = place(glyphs[ord('?')])
    records = []
    for first, last in RANGES:
        for code in range(first, last + 1):
            if code in glyphs:
                records.append(place(glyphs[code]))
            elif code in fallback:
                records.append(place(fallback[code]))
            else:
                records.append(question)

    out = bytearray(b'OFN2')
    out += struct.pack('<BBH', ascent + descent, ascent, len(RANGES))
    for first, last in RANGES:
        out += struct.pack('<IHH', first, last - first + 1, 0)
    for offset, w, h, xo, yo, adv in records:
        out += struct.pack('<IBBbbB3x', offset, w, h, xo, yo, adv)
    out += bitmaps
    return bytes(out)


def subset(src, dst):
    """Write a copy of the BDF @p src with only the RANGES characters."""
    with open(src, encoding='utf-8') as f:
        lines = f.read().split('\n')
    head, chars, i = [], [], 0
    while not lines[i].startswith('CHARS'):
        head.append(lines[i])
        i += 1
    glyph = None
    for line in lines[i + 1:]:
        if line.startswith('STARTCHAR'):
            glyph = [line]
        elif glyph is not None:
            glyph.append(line)
            if line == 'ENDCHAR':
                code = int(next(l for l in glyph if l.startswith('ENCODING')).split()[1])
                if wanted(code):
                    chars.append(glyph)
                glyph = None
    head.insert(1, 'COMMENT Subset for the Omega kernel (tools/make_font.py RANGES)')
    with open(dst, 'w', encoding='utf-8') as f:
        f.write('\n'.join(head + [f'CHARS {len(chars)}'] + [l for g in chars for l in g] +
                          ['ENDFONT', '']))


def read_manifest():
    """Lines of fonts.txt: output-name source.bdf [fallback.bdf]."""
    entries = []
    with open(os.path.join(FONT_DIR, 'fonts.txt')) as f:
        for line in f:
            line = line.split('#', 1)[0].split()
            if line:
                entries.append((line[0], line[1], line[2] if len(line) > 2 else None))
    return entries


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--all', action='store_true', help='process assets/fonts/fonts.txt')
    ap.add_argument('--check', action='store_true', help='with --all: only verify')
    ap.add_argument('--subset', nargs=2, metavar=('IN', 'OUT'), help='subset a BDF font')
    args = ap.parse_args()

    if args.subset:
        subset(*args.subset)
        return 0
    if not args.all:
        ap.error('--all or --subset required')

    stale = []
    src = os.path.join(FONT_DIR, 'src')
    for name, source, fallback in read_manifest():
        data = convert(os.path.join(src, source), fallback and os.path.join(src, fallback))
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
