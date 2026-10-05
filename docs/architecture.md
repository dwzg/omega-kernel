# Architecture

The kernel is a single GBA program (`ezkernel.gba`, about 400 KB) that runs
from the cartridge's own flash. It is written in C17 with a little ARM/Thumb
assembly, and organised in layers. Each layer only uses the ones below it.

```
            ┌───────────────────────────────────────────────┐
            │ main.c          start-up checks, then the UI  │
            ├───────────────────────────────────────────────┤
            │ ui/             screens and widgets           │
            ├──────────────────────┬────────────────────────┤
            │ loader/              │ gfx/                   │
            │ files, NOR library,  │ drawing, fonts         │
            │ starting games       │                        │
            ├──────────┬───────────┴───────┬────────────────┤
            │ patch/   │ core/             │ platform/      │
            │ patcher  │ portable logic    │ machine API    │
            ├──────────┴───────────────────┴────────────────┤
            │ hal/            cartridge hardware            │
            ├───────────────────────────────────────────────┤
            │ third_party/fatfs, libgba                     │
            └───────────────────────────────────────────────┘
```

## Modules

| Directory | Responsibility | Runs on a PC? |
| --- | --- | --- |
| `src/hal/` | Hardware access: FPGA registers (`omega`), SD card (`sd`), NOR flash (`nor`), the S71 configuration flash (`config_flash`), SRAM, RTC, reset stubs, FatFs glue (`diskio`) | no |
| `src/core/` | Pure logic with no hardware dependencies: settings encoding, save types, the cheat parser, paths, dates, the recently played list, list navigation, CRC-32 | yes |
| `src/patch/` | The patch engine that adds add-ons and game fixes to ROMs, its tables and the assembly payloads | engine yes, payloads no |
| `src/loader/` | Everything with files and boot: directory listing, FAT map, ROM loading, saves, NOR library, firmware update, `boot.c` | partly (file code yes) |
| `src/gfx/` | Mode 3 drawing primitives, text, off-screen drawing | yes |
| `src/ui/` | The screens of the interface, see [User interface](user-interface.md) | yes |
| `src/platform/` | The small API the UI needs from the machine (`platform.h`): input, vblank, clock, settings storage. `gba/` implements it for the console | API yes, `gba/` no |
| `src/data/` | Generated tables: save-type database (2,815 games) and IRQ patch database | yes |

"Runs on a PC" is what the [tests](development.md) build: the UI simulator
runs `ui/`, `gfx/`, `core/` and the file parts of `loader/` with FatFs on a disk
image, replacing `hal/`, `boot.c` and `nor_games.c` with stubs.

## Start-up

`main()` (`src/main.c`):

1. sets up interrupts and the display, switches the SD interface off and
   the RTC on;
2. offers a firmware update if the cartridge's FPGA is older than the
   bundled image;
3. shows a plain start-up screen and mounts the SD card (fatal error if this fails);
4. loads the settings and the NOR game table;
5. runs the interface, starting in the SD card browser. It never returns:
   starting a game resets the GBA into the game.

## Memory

The GBA has 32 KB of fast IWRAM and 256 KB of EWRAM. Both are nearly full,
so memory use is planned:

| Region | Use |
| --- | --- |
| IWRAM (`.iwram`, `.data`, `.bss`) | Code that must not run from ROM (below), stacks, small globals |
| EWRAM | Large buffers (`loader/buffers.c`): 128 KB scratch buffer shared by ROM loading, NOR writing, thumbnails and the cheat menu; cheat codes; directory listing (768 entries); NOR table; the patch context |
| VRAM `0x06000000` | The 240 × 160 frame buffer (mode 3) |
| VRAM `0x06012C00`-`0x06018000` | Off-screen buffer for the menu's list rows; while a game is started, the work buffer where patch payloads are assembled |

The link fails if static data no longer fits (`tools/memory_limits.ld`);
`make size` prints the use. At the time of writing, 852 bytes of EWRAM are free.

**The scratch buffer** (`g_scratch`) may be used by any module, but nobody may
expect its contents to survive a call into another module.

## Code placement rules

Several hardware operations make the cartridge ROM, and with it the running
kernel, temporarily unreadable: FPGA register writes, SD transfers, writing
NOR or the configuration flash, switching the ROM page. Code that does this
must run from IWRAM:

- Mark such functions `IWRAM_CODE` (from libgba) and keep them in `src/hal/`.
- Helpers they call must be `ALWAYS_INLINE` or also in IWRAM.
- Calls from IWRAM back into ROM are only allowed where the ROM is known to be
  mapped. `tools/check_iwram_calls.py` runs on every build and fails it
  otherwise; its allow-list documents the exceptions (`delay_loop`, whose
  timing was tuned in ROM, and the drawing helpers).
- Assembly routines called from C must be marked `.thumb_func`;
  `tools/check_thumb_entries.py` checks this on every build.

## Error handling

Functions that can fail return a result (`bool` or a `*_result_t` enum); there
is no global error state. User-facing messages are made in one place per
area, for example `boot_result_message()`. The kernel never silently ignores a
failed SD read: loading stops and the message is shown.

## Compatibility constraints

The kernel shares its on-card and in-flash formats with the original kernel
(see [SD card layout](sd-card-layout.md)). Structures stored in flash have
static assertions on their size; don't change them.

## Naming and style

- `snake_case` for functions and variables, with a module prefix:
  `nor_game_write()`, `settings_decode()`, `ui_list_draw()`.
- Types end in `_t`; constants and macros are `UPPER_CASE`.
- Globals are rare, prefixed `g_` and declared in a header; file-local state is
  `static` and prefixed `s_`.
- Public functions are documented in their header with Doxygen comments.
- Formatting is enforced with clang-format (`make format`).

See [Development](development.md) for the full workflow.
