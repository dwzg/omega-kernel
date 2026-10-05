# Boot process

What happens between choosing **Play** and the game's first frame. The code is
`src/loader/boot.c`; the hardware side is in [Hardware](hardware.md).

## Overview

The FPGA, not the kernel, reads the game from the SD card and writes saves
back to it. The kernel's job is to tell the FPGA *where* the files are, to
prepare save memory, optionally to modify the game, and finally to reset the
GBA into it.

```
Play ──► map ROM file ──► save type ──► save file ──► recent list
                                                         │
          ┌──────────────────────────────────────────────┘
          ▼
   Play (clean)                Play + add-ons
   FPGA copies ROM             .pat cache? ── yes ──► FPGA copies ROM
   game fixes                      │ no
          │                    IRQ database? ── yes ──► FPGA copies ROM
          │                        │ no
          │                    kernel copies ROM, scanning it
          │                    add-ons + game fixes, store .pat
          ▼                        ▼
       omega_boot(PSRAM page, RTC setting, BIOS boot?)
```

## Starting a game from the SD card

`boot_sd_game()`:

1. **Read the ROM header** for the game code and check the size (≤ 32 MB).
2. **Map the ROM file** into the [FAT map](#the-fat-map).
3. **Store the save-type choice** (`.mde`) if it changed, and resolve the
   save type: the choice, or the database entry for the game code.
4. **Prepare the save file** in `/SAVER`: create it filled with `0xFF` if it
   doesn't exist, add it to the FAT map, and copy its contents into
   cartridge SRAM (pages `0x00` and `0x10`).
5. **Remember the game** in the recently played list.
6. **Write the FAT map parameters**: ROM size, copy mode, sectors per
   cluster, save type and size.
7. Then, depending on the choice:
   - **Play**: upload the FAT map and let the FPGA copy the ROM into PSRAM.
     Apply game-specific fixes that don't need hooks (PocketNES builds, a few
     games with known bugs on flash carts), set auto-save and boot.
   - **Play + add-ons**: prepare the save-state file if needed, then get the
     patch locations from the `.pat` cache, the built-in IRQ database (with
     *Fast patching*), or by copying the ROM in software and scanning every
     block. Install the add-ons ([Patching](patching.md)), store the cache if
     one was made, and boot.
8. **`omega_boot()`** switches the ROM page to PSRAM, sets the RTC register,
   clears RAM and registers with the BIOS and resets.

Errors at any step (file can't be read, too fragmented, save can't be
created) stop the boot and are shown on screen. The game itself is never
changed on the SD card; patches only exist in PSRAM or NOR.

## Starting a game from NOR flash

`boot_nor_game()` is shorter, because the game is already in place and
patched:

1. Resolve the save type (the `.mde` choice of the original file name, or
   the database).
2. Prepare the save file as above.
3. If the game was written with the save-state add-on, prepare its `.rts` file.
4. Upload the FAT map with copy mode 2 ("ROM already loaded").
5. `omega_boot()` with the game's ROM page from the NOR table.

## Copying a game to NOR

`copy_to_nor()` and `nor_game_write()` (`src/loader/nor_games.c`):

1. Find the end of the games already in NOR and check there is room
   (the game's size rounded up to 128 KB, plus one block if the add-on code
   doesn't fit into the last one).
2. With add-ons: find the trim size and the game-specific fixes.
3. For each 128 KB block: read it from the SD card into the scratch buffer,
   look for IRQ references in it, apply the changes that fall into this
   block, erase the NOR block and program it.
4. Add the game to the NOR table and save the table.

## The FAT map

The FPGA needs the physical location of the ROM, the save file and the save
state on the card. The kernel sends a 1 KB table (`src/loader/fat_map.c`):

| Offset | Contents |
| --- | --- |
| `0x000` | ROM runs |
| `0x1F0` | ROM size in bytes |
| `0x1F4` | Copy mode: 1 = FPGA copies the ROM into PSRAM, 2 = already loaded |
| `0x1F8` | Sectors per cluster |
| `0x1FC` | Save type (bits 24-31) and save size in bytes (bits 0-23) |
| `0x200` | Save file runs |
| `0x300` | Save-state file runs |

Each run is two 32-bit words, `{offset in the file in sectors, first sector
on the card}`, for a stretch of consecutive clusters. A list ends with
`{0xFFFFFFFF, 0}`. The ROM area holds up to 62 runs, the other two up to 32 —
a more fragmented file can't be started ("The file is too fragmented").

The kernel builds the runs by following the FAT cluster chain with FatFs's
internal `Get_NextCluster()` and `ClustToSect()`, exported by a small patch to
FatFs (see `third_party/fatfs/README.md`).

## Reset

`src/hal/reset.s`:

- **Direct start** (`reset_soft`): copies a small stub to EWRAM, clears IWRAM
  except the routine that is clearing it, then the stub disables interrupts,
  tells the BIOS to start from ROM and calls `SoftReset`.
- **BIOS start** (`reset_hard`, hold **L** while pressing **A**): `HardReset`,
  which shows the Nintendo logo and runs the full BIOS start-up.

Both run from IWRAM, because the ROM page has already been switched to the
game when they are called.
