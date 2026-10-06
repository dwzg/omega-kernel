# SD card layout and file formats

Everything the kernel reads or writes, on the SD card and in the cartridge's
own flash. All formats are those of the original EZ-FLASH kernel, so cards
and cartridges can be moved between the two.

## Folders

```
/                         your games (*.gba), in any folders you like
├── SAVER/                save files, save-type choices, recently played
│   ├── <game>.sav
│   ├── <game>.bak        the save from before the last start
│   ├── <game>.mde
│   ├── Favorites.txt
│   └── Recently play.txt
├── RTS/                  save states: <game>.rts
├── PATCH/                patch caches: <game>.pat
├── CHEAT/                cheat files
│   ├── <game>.cht        your own, named like the game file
│   ├── GameID2cht.bin    index of the cheat library
│   └── Eng/0000 ... Eng/2800/<number>.cht
└── IMGS/<c1>/<c2>/<code>.bmp   box art
```

`<game>` is the game's file name with its extension replaced: the save file of
`/GBA/RPG/Golden Sun.gba` is `/SAVER/Golden Sun.sav`. Games with the same file
name in different folders therefore share their save.

Paths are limited to 255 characters. File names may be long names in any
language. The kernel shows Latin letters with accents, Greek, Cyrillic and
Japanese (kana and about 6,400 kanji); other characters are shown as `?`. A name that would
make the whole path longer than 255 bytes is opened through its 8.3 short
name, but its save and other files still use the full name.

## Save files (`/SAVER/*.sav`)

A raw image of the game's save memory, as any emulator writes it:

| Save type | File size |
| --- | --- |
| EEPROM 512 B | 512 B |
| EEPROM 8 KB | 8 KB |
| SRAM | 32 KB |
| SRAM 64 KB (unknown games) | 64 KB |
| Flash 64 KB | 64 KB |
| Flash 128 KB | 128 KB |

If a game has no save file, one is created filled with `0xFF` (erased
memory). The FPGA writes changes back to this file while the game runs, so
the file must stay in one piece on the card: the kernel tells the FPGA its
location sector by sector (see [Boot process](boot-process.md#the-fat-map)),
which only works if it has at most 32 fragments.

## Save-type choices (`/SAVER/*.mde`)

16 bytes. Byte 0 holds the choice made on the game page:

| Value | Choice |
| --- | --- |
| 0 | Auto (look the game up in the database) |
| 1 | SRAM |
| 2 | EEPROM 8K |
| 3 | EEPROM 512 |
| 4 | Flash 64K |
| 5 | Flash 128K |

The file is written when the choice changes, and when the save type of a
game that is not in the database has been detected. A missing file means
Auto. The other bytes hold the detection result (the original kernel ignores
them):

| Byte | Meaning |
| --- | --- |
| 1 | `D` when bytes 2–11 are valid |
| 2 | Detected save mode (the FPGA code, e.g. `0x31` = Flash 128K, `0x10` = none found) |
| 4–7 | Game code the result belongs to |
| 8–11 | ROM size it belongs to (little-endian); a different file is scanned again |

## Recently played (`/SAVER/Recently play.txt`)

Plain text, one absolute path per line, newest first, at most ten lines.
Lines that don't start with `/` are ignored.

## Favorites (`/SAVER/Favorites.txt`)

Plain text, one absolute path per line, at most 64 lines, in the order they
were added (the screen sorts them by name). Lines that don't start with `/`
are ignored.

## Save states (`/RTS/*.rts`)

448 KB (`0x70000` bytes) per game, created on the first start with the save
state add-on. The FPGA stores the complete state of the GBA in it (RAM,
registers, video memory, save memory). Like save files, it must have at most
32 fragments.

## Patch caches (`/PATCH/*.pat`)

When a game is started with add-ons and isn't in the built-in patch
database, the kernel scans it for the places it must change and stores the
result here. The file is 320 bytes of little-endian 32-bit words:

| Words | Contents |
| --- | --- |
| 0-63 | 32 pairs `{word index, new value}` of changes to the ROM |
| 64 | NOR mode (always 0 in a cache) |
| 65 | Window offset |
| 66 | PocketNES variant (0, 1 or 2) |
| 67 | PocketNES entry index |
| 68 | Word offset |
| 69 | Number of used pairs |
| 70 | Trim size: where the add-on code is placed |
| 71 | Offset of the ROM's original entry branch |
| 72-75 | Add-ons the cache was made for: return to menu, save states, sleep, cheats |
| 76-79 | 0 |

A cache made for a different set of add-ons is ignored and rebuilt. Deleting
`/PATCH` is always safe. See [Patching](patching.md).

## Cheat files

### `.cht` files

Text files in the format of the original kernel:

```ini
[Infinite HP]
ON=2000500,E7,03;

[Party Level]
10=2000500,0A;
50=2000500,32;
99=2000500,63;

[GameInfo]
Name=Golden Sun
System=GBA
```

- Each `[section]` is a cheat; each `key=value` line in it is one option.
  At most one option per cheat can be selected.
- A value is a list of codes separated by `;`. A code is a hexadecimal address
  followed by one or more hexadecimal bytes: `2000500,E7,03` writes `E7` to
  `0x2000500` and `03` to `0x2000501`. Values may continue on following lines.
- Addresses below `0x40000` are in EWRAM (`0x02000000 + address`); higher ones
  are IWRAM (`0x03000000 + (address & 0x7FFF)`).
- `[GameInfo]` and everything after it is not shown; its `Name` is the title
  of the cheat screen.
- Lines starting with `#` or `//`, and `/* ... */` blocks, are comments. A line
  starting with `--` ends the list.
- Spaces in names are ignored when matching options, as in the original kernel.

Up to 2,271 codes can be active at once (the space left in the add-on code).

### The cheat library

`/CHEAT/GameID2cht.bin` is a list of 8-byte records: the 4-character game code
followed by a 4-character library number (as ASCII digits, e.g. `0042`). The
cheat file for number *n* is `/CHEAT/Eng/<bucket>/<n>.cht`, where the bucket
is *n* rounded down to a multiple of 200 (at most 2800), written with four
digits: number `1234` is `/CHEAT/Eng/1200/1234.cht`.

## Thumbnails

`/IMGS/<c1>/<c2>/<code>.bmp`, where `<code>` is the 4-character game code and
`<c1>`, `<c2>` its first two characters. The file is 19,256 bytes: a 54-byte
header (ignored) followed by 120 × 80 pixels, top row first, each a
little-endian 16-bit GBA colour (`0bbbbbgggggrrrrr`). The kernel shows them at
full size on the game page.

## Cartridge flash

The kernel itself lives in the cartridge's S71 flash. Two 64 KB blocks after
it hold kernel data:

| Offset | Size | Contents |
| --- | --- | --- |
| `0x7A0000` | 8,448 B | NOR game table: 64 entries of 132 bytes |
| `0x7B0000` | 512 B | Settings: 256 16-bit words |

### Settings

| Word | Meaning | Values (anything else means the default) |
| --- | --- | --- |
| 0 | Language | `0xE1E1` = English (always written) |
| 1 | Return-to-menu add-on | 0 / 1 (default 0) |
| 2 | Save-state add-on | 0 / 1 (default 0) |
| 3 | Sleep add-on | 0 / 1 (default 0) |
| 4 | Cheats | 0 / 1 (default 0) |
| 5-7 | Sleep (save state) hotkey | button numbers 0-9 (default L, R, SELECT) |
| 8-10 | Menu (load state) hotkey | button numbers 0-9 (default L, R, START) |
| 11 | Fast patching | 0 / 1 (default 1) |
| 12 | Thumbnails (original kernel) | kept, unused |
| 13 | Clock for games | 0 / 1 (default 1) |

Button numbers are bit positions in the GBA's KEYINPUT register:
A 0, B 1, SELECT 2, START 3, Right 4, Left 5, Up 6, Down 7, R 8, L 9.
Words 14-255 are preserved.

### NOR game table

One 132-byte entry per game in NOR flash, in the order they were written:

| Offset | Size | Contents |
| --- | --- | --- |
| 0 | 100 | File name the game was copied from |
| 100 | 2 | ROM page register value that maps the game |
| 102 | 2 | 1 if written with add-ons |
| 104 | 2 | 1 if the save-state add-on was enabled |
| 106 | 2 | reserved |
| 108 | 4 | Size in NOR (multiple of 128 KB) |
| 112 | 4 | reserved |
| 116 | 16 | ROM header bytes `0xA0-0xAF` (title and game code) |

At start-up the kernel walks the NOR flash and accepts an entry only if the
ROM header found there matches, so an erased chip shows no games even if
the table wasn't cleared.
