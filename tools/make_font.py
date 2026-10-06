#!/usr/bin/env python3
"""Convert BDF bitmap fonts into the kernel's font format.

The kernel uses pixel fonts: every pixel of a glyph is either on or off, so
text is sharp on the GBA screen. The sources are BDF files (a plain text
bitmap format) in assets/fonts/src/; this script packs the characters the
kernel can show (RANGES, and CJK_RANGES for fonts marked +cjk) into the
compact "OFN3" format it reads in place.

File format (little-endian), see src/gfx/font.c:

    offset  size  field
    0       4     magic "OFN3"
    4       1     line height in pixels
    5       1     ascent (baseline position from the top of a line)
    6       2     number of ranges R
    8       8*R   ranges, sorted: runs of consecutive code points the font has
                    u32 first code point, u16 count,
                    u16 index of the run's first glyph record
    ...     12*n  glyph records, for every code point of every range:
                    u32 bitmap offset (from the start of the bitmap area)
                    u8  width, u8 height
                    s8  x offset, s8 y offset (from the top of the line)
                    u8  advance, 3 bytes reserved
    ...           bitmaps: 1 bit per pixel, rows padded to whole bytes,
                  most significant bit = leftmost pixel

A character missing from a font is taken from its fallback font (if one is
given in fonts.txt). The kernel itself can also fall back to another font at
run time (the title font uses the body font's kanji), otherwise it draws '?'.

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

# Kanji and the rest of the CJK punctuation, for fonts marked +cjk.
CJK_RANGES = [
    (0x3004, 0x303F),  # CJK symbols (々, 〜, brackets)
    (0x4E00, 0x9FFF),  # CJK Unified Ideographs (Galmuri has the JIS kanji)
]


def wanted(code, cjk=True):
    ranges = RANGES + (CJK_RANGES if cjk else [])
    return any(first <= code <= last for first, last in ranges)


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


def convert(path, fallback_path=None, cjk=False):
    ascent, descent, glyphs = read_bdf(path)
    fallback = {}
    if fallback_path:
        f_ascent, f_descent, fallback = read_bdf(fallback_path)
        if (f_ascent, f_descent) != (ascent, descent):
            sys.exit(f'{fallback_path}: metrics differ from {path}')

    codes = sorted(c for c in set(glyphs) | set(fallback) if wanted(c, cjk))
    if ord('?') not in glyphs:
        sys.exit(f"{path}: no '?' glyph")

    runs = []  # [first, count, base]
    for i, code in enumerate(codes):
        if runs and runs[-1][0] + runs[-1][1] == code:
            runs[-1][1] += 1
        else:
            runs.append([code, 1, i])
    if len(codes) > 0xFFFF:
        sys.exit(f'{path}: too many glyphs')

    bitmaps = bytearray()
    records = []
    for code in codes:
        advance, (w, h, xoff, yoff), rows = glyphs.get(code) or fallback[code]
        offset = len(bitmaps)
        bitmaps.extend(pack_rows(rows, w, h))
        # BDF offsets are from the baseline (y up); ours from the line top.
        records.append((offset, w, h, xoff, ascent - (h + yoff), advance))

    out = bytearray(b'OFN3')
    out += struct.pack('<BBH', ascent + descent, ascent, len(runs))
    for first, count, base in runs:
        out += struct.pack('<IHH', first, count, base)
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
    """Lines of fonts.txt: output-name source.bdf [fallback.bdf] [+cjk]."""
    entries = []
    with open(os.path.join(FONT_DIR, 'fonts.txt')) as f:
        for line in f:
            words = line.split('#', 1)[0].split()
            if words:
                files = [w for w in words[1:] if not w.startswith('+')]
                entries.append((words[0], files[0], files[1] if len(files) > 1 else None,
                                '+cjk' in words))
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
    for name, source, fallback, cjk in read_manifest():
        data = convert(os.path.join(src, source), fallback and os.path.join(src, fallback), cjk)
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
