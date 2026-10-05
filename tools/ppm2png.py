#!/usr/bin/env python3
"""Convert binary PPM (P6) screenshots from the UI simulator to PNG.

Each input.ppm is written as input.png next to it (and the PPM removed).
An optional --scale N enlarges the image by pixel repetition. Uses only
the Python standard library.
"""
import struct
import sys
import zlib
import os


def chunk(kind, data):
    body = kind + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


def convert(path, scale):
    data = open(path, "rb").read()
    parts = data.split(b"\n", 3)
    if parts[0] != b"P6" or parts[2] != b"255":
        raise ValueError(f"{path}: not a binary PPM")
    width, height = map(int, parts[1].split())
    pixels = parts[3]
    rows = bytearray()
    for y in range(height):
        line = pixels[y * width * 3:(y + 1) * width * 3]
        if scale > 1:
            line = b"".join(line[x * 3:x * 3 + 3] * scale for x in range(width))
        for _ in range(scale):
            rows += b"\0" + line
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width * scale, height * scale, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(rows), 9))
    png += chunk(b"IEND", b"")
    out = os.path.splitext(path)[0] + ".png"
    open(out, "wb").write(png)
    os.remove(path)


def main():
    args = sys.argv[1:]
    scale = 1
    if args[:1] == ["--scale"]:
        scale, args = int(args[1]), args[2:]
    for path in args:
        convert(path, scale)


if __name__ == "__main__":
    main()
