#!/usr/bin/env python3
"""Build the SD card image used by the UI simulator.

Creates a small FAT32 image with mkfs.vfat and fills it with mtools, so no
root rights or loop devices are needed. The games are tiny fake ROMs with a
real-looking header (title and game code); the kernel only reads headers in
the simulator.

The ATTR folder covers the file-attribute regression: games without the
archive bit, read-only games, hidden and system files.

Usage: make_sd_image.py output.img
"""
import os
import shutil
import struct
import subprocess
import sys
import tempfile

GAMES = [
    # (path, header title, game code, size)
    ("Metroid Fusion.gba", "METROID4USA", "AMTE", 8 << 20),
    ("Golden Sun.gba", "Golden_Sun_A", "AGSE", 8 << 20),
    ("Pokemon - Emerald Version.gba", "POKEMON EMER", "BPEE", 16 << 20),
    ("Advance Wars.gba", "ADVANCEWARS", "AWRE", 4 << 20),
    ("Castlevania - Aria of Sorrow.gba", "CASTLEVANIA2", "A2CE", 8 << 20),
    ("Mario & Luigi - Superstar Saga.gba", "MARIO&LUIGI", "A88E", 16 << 20),
    ("The Legend of Zelda - The Minish Cap (Europe) (En,Fr,De,Es,It).gba", "GBAZELDA MC", "BZMP",
     16 << 20),
    ("Homebrew Demo.gba", "", "\0\0\0\0", 256 << 10),
    ("GBA/Puzzle/Tetris Worlds.gba", "TETRISWORLDS", "ATWE", 4 << 20),
    ("GBA/Puzzle/Puyo Pop.gba", "PUYOPOP", "APUE", 4 << 20),
    ("GBA/RPG/Final Fantasy VI Advance.gba", "FF6ADVANCE", "BZ6E", 8 << 20),
    ("ATTR/Normal.gba", "NORMAL", "ZNOR", 1 << 20),
    # Names beyond ASCII: shown with their accents and kana; a name too long
    # for the browser is opened through its 8.3 short name.
    ("GBA/World/Pokémon - Version Émeraude.gba", "POKEMON EMER", "BPEF", 1 << 20),
    ("GBA/World/ポケットモンスター エメラルド.gba", "POKEMON EMER", "BPEJ", 1 << 20),
    ("GBA/World/Jeux Français/Astérix & Obélix XXL.gba", "ASTERIX", "BLXP", 1 << 20),
    ("GBA/World/ファイナルファンタジータクティクスアドバンス (Japan) (Rev 1) (Translated) [v1.2].gba",
     "FFTA", "AFXJ", 1 << 20),
    ("ATTR/No Archive Bit.gba", "NOARCHIVE", "ZNOA", 1 << 20),
    ("ATTR/Read Only.gba", "READONLY", "ZRDO", 1 << 20),
    ("ATTR/Hidden.gba", "HIDDEN", "ZHID", 1 << 20),
    ("ATTR/System.gba", "SYSTEM", "ZSYS", 1 << 20),
]

OTHER_FILES = {
    "notes.txt": b"Not a game.\r\n",
    "._Metroid Fusion.gba": b"\0" * 64,  # macOS resource fork
    "SAVER/Recently play.txt": b"/Golden Sun.gba\r\n/GBA/RPG/Final Fantasy VI Advance.gba\r\n"
                               b"/Metroid Fusion.gba\r\n",
    "CHEAT/Golden Sun.cht": (
        b"[Infinite HP]\r\nON=2000500,E7,03;\r\n\r\n"
        b"[Max Coins]\r\nON=2000250,9F,86,01;\r\n\r\n"
        b"[Party Level]\r\n10=2000500,0A;\r\n50=2000500,32;\r\n99=2000500,63;\r\n\r\n"
        b"[Walk Through Walls]\r\nON=3001234,01;\r\n\r\n"
        b"[GameInfo]\r\nName=Golden Sun\r\n"),
}

ATTRIBUTES = [
    # (mattrib arguments, path) -- applied after copying
    ("-a", "ATTR/No Archive Bit.gba"),
    ("+r", "ATTR/Read Only.gba"),
    ("+h", "ATTR/Hidden.gba"),
    ("+s", "ATTR/System.gba"),
]


def rom_header(title, code):
    header = bytearray(0xC0)
    header[0:4] = struct.pack("<I", 0xEA00002E)
    header[0xA0:0xAC] = title.encode("ascii")[:12].ljust(12, b"\0")
    header[0xAC:0xB0] = code.encode("latin-1")
    header[0xB2] = 0x96
    return bytes(header)


def thumbnail():
    """120x80 cover art in the kernel's thumbnail format (BMP header + RGB555)."""
    pixels = bytearray()
    for y in range(80):
        for x in range(120):
            r, g, b = 4 + y // 6, 10 + x // 10, 24 - y // 8
            if (x - 60) ** 2 + (y - 40) ** 2 < 18 ** 2:
                r, g, b = 31, 22, 6
            pixels += struct.pack("<H", r | (g << 5) | (b << 10))
    return b"BM" + b"\0" * 52 + pixels


def main():
    image = sys.argv[1]
    stage = tempfile.mkdtemp()
    try:
        files = {}
        for path, title, code, size in GAMES:
            # Sparse files: only the header is real.
            files[path] = (rom_header(title, code), size)
        for path, data in OTHER_FILES.items():
            files[path] = (data, len(data))
        files["IMGS/A/M/AMTE.bmp"] = (thumbnail(), 0x4B38)

        for path, (data, size) in files.items():
            full = os.path.join(stage, path)
            os.makedirs(os.path.dirname(full), exist_ok=True)
            with open(full, "wb") as f:
                f.write(data)
                f.truncate(size)

        if os.path.exists(image):
            os.remove(image)
        subprocess.run(["mkfs.vfat", "-C", "-F", "32", "-i", "0E2A0001", "-n", "OMEGA", image,
                        str(160 * 1024)], check=True, stdout=subprocess.DEVNULL)
        # A UTF-8 locale makes mtools store non-ASCII names as long names.
        env = dict(os.environ, MTOOLS_SKIP_CHECK="1", LC_ALL="C.UTF-8")
        top = sorted(os.listdir(stage))
        subprocess.run(["mcopy", "-s", "-i", image] + [os.path.join(stage, t) for t in top] +
                       ["::/"], check=True, env=env)
        for args, path in ATTRIBUTES:
            subprocess.run(["mattrib", "-i", image, args, "::/" + path], check=True, env=env)
    finally:
        shutil.rmtree(stage)


if __name__ == "__main__":
    main()
