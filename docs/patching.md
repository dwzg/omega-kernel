# Patching

How the kernel adds the in-game add-ons (return to menu, save states, sleep,
cheats) and game-specific fixes to a ROM. The engine is `src/patch/patch.c`;
it is hardware-independent and unit-tested on a PC
(`tests/unit/test_patch.c`).

## The idea

All add-ons work the same way: a small piece of code (a *payload*) is placed
in unused space at the end of the ROM, and the game is changed so that the
payload runs on every interrupt. The payload checks the buttons, does its
work (opens the menu, sleeps, applies cheats) and continues into the game's
own interrupt handler.

Two changes make the payload run:

1. **Entry point.** The first word of a GBA ROM is a branch to the game's
   start code. It is replaced by a branch to the payload, which installs
   itself and jumps to the original start address.
2. **IRQ vector.** Games install their interrupt handler by writing its
   address to `0x03007FFC` (or its mirror `0x03FFFFFC`). Every literal in the
   ROM that holds one of these addresses is changed to `0x03007FF4`, so the
   game writes its handler where the payload can find it, and the payload owns
   the real vector.

## Finding the IRQ references

Three sources, tried in order:

| Source | When | Speed |
| --- | --- | --- |
| `.pat` cache | The game was patched before with the same add-ons | instant |
| IRQ database (`src/data/irq_patch_db.c`) | *Fast patching* on, game code in the table | instant |
| Scan | Otherwise: the kernel copies the ROM block by block and looks at every word | seconds; result cached |

A list of extra, game-specific changes (`ROM_IRQ_FIXES` in
`src/patch/rom_fixes.c`) is always added; some games keep their vector in
unusual ways.

## Where the payload goes: trim size

The payload needs 0x300 bytes (reset/sleep), 0x1000 (save states) or 0x2000
(with cheats). `patch_find_trim_size()` looks backwards from the end of the
ROM for the last byte that differs from the ROM's final byte (the padding,
usually `0x00` or `0xFF`) and places the payload just after it, aligned to 16 bytes. If
there is no padding, the payload goes after the ROM.

Limits keep the payload where the game can reach it:

- a ROM of up to 8 MB keeps the payload below 8 MB, up to 16 MB below 16 MB;
- EEPROM games below 16 MB keep it below `0x1000000 - size`, because the
  EEPROM is mapped above 16 MB;
- in NOR the payload must not cross a 128 KB block boundary;
- `ROM_TRIM_OVERRIDES` sets fixed positions for games where the search fails.

## Payloads

| Payload | Source | Used when |
| --- | --- | --- |
| Reset / sleep | `payloads/gba_sleep_patch.s` | Only *return to menu* and/or *sleep* |
| Save states only | `payloads/gba_rts_only.s` | *Save states* is the only add-on |
| Full | `payloads/gba_rts_patch.s` | *Save states* or *cheats* with others |
| Fire Emblem | `payloads/Fire_Emblem_*.s` | Five Fire Emblem releases need their own save-state code |

Payloads are copied into a VRAM work buffer, their parameter words are filled
in, and the result is written into the game:

- return address: the game's original entry point;
- the two hotkeys as KEYINPUT masks (`0` disables one);
- the save-state switch;
- for the full payload, the cheat table: `{address, value}` pairs, with
  addresses mapped to EWRAM (`0x02000000`) or IWRAM (`0x03000000`).

For save states, the game's stack is moved down by 0x80 bytes to make room
for the payload's state (the engine finds the stack set-up instruction near
the entry point).

## Game-specific fixes

Some are applied on every start, also without add-ons:

| Fix | Why |
| --- | --- |
| PocketNES | PocketNES-built NES games need an IRQ fix (`assets/patches/pocketnes_irq.bin`) |
| Write fixes | A few games (Dragon Ball Z, Top Gun) contain code that misbehaves on flash carts |
| Fire Emblem | Their save routines are redirected to the matching payload, and auto-save is disabled for them |
| IWRAM size | One game's IWRAM size word collides with the payload |

## PSRAM and NOR

The engine writes through a small platform interface (`patch_platform_t`):

- **PSRAM** (`patch_apply_*_psram`): the whole game is in PSRAM; changes are
  written through the paged PSRAM window.
- **NOR** (`patch_apply_*_nor`): the game is written one 128 KB block at a
  time. The engine is called once per block and applies only the changes that
  fall into it, so the same state produces the same result as in PSRAM.

## The `.pat` cache

After a scan, the engine's state (the change list, trim size, entry branch,
PocketNES data and the add-ons it was made for) is stored in `/PATCH`. The
format is in [SD card layout](sd-card-layout.md#patch-caches-patchpat).
