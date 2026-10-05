# Development

How to work on the kernel: tests, the UI simulator, code style and CI.
See [Building](building.md) for the toolchain and [Architecture](architecture.md)
for how the code is organised.

## Tests

```sh
make test
```

builds and runs everything under `tests/` with the PC's C compiler, with
AddressSanitizer and UndefinedBehaviorSanitizer enabled. It needs a C compiler
(GCC or Clang), Python 3, `mtools` and `dosfstools`; not devkitPro.

| Part | What it covers |
| --- | --- |
| `tests/unit/` | Unit tests of `core/`, `patch/` and `gfx/`: settings, dates, paths, save types, the cheat parser, list navigation, the patch engine with fake payloads, fonts and drawing |
| `tests/sim/` | The UI simulator (below) |

`tests/check.h` is a minimal test framework: `TEST(name)` defines a test,
`CHECK`, `CHECK_EQ`, `CHECK_STR` and `CHECK_MEM` check values, and each file
ends with a `SUITE` that runs its tests. `tests/build/unit_tests <filter>` runs
only tests whose name contains `filter`.

Options: `make -C tests CC=clang`, `make -C tests SANITIZE=0`.

### The UI simulator

The simulator runs the real interface code (`ui/`, `gfx/`, `core/` and the
file parts of `loader/`) with the real FatFs, on a PC:

- `tests/sim/make_sd_image.py` builds a FAT32 image with games, folders, a
  cheat file, box art and files with unusual attributes;
- `platform_sim.c` provides the frame buffer, scripted button presses and a
  fixed clock;
- `stubs.c` stands in for starting games, the NOR flash and firmware updates.

Scenarios in `tests/sim/scenarios.c` walk through every screen and take
screenshots. Each screenshot's CRC is compared with `tests/sim/golden.txt`;
differing screens are written to `tests/build/screens-failed/`. The simulator
also checks file listing directly, for example that games without the
archive attribute or marked read-only are listed.

```sh
make -C tests sim            # run the scenarios
make -C tests screenshots    # PNGs of every screen in tests/build/screens/
make -C tests update-golden  # accept the current screens (look at them first!)
```

CI uploads the screenshots of every run as an artifact.

### What is not covered

Code that talks to the cartridge (`hal/`, `boot.c`, `nor_games.c`,
`rom_loader.c`, `firmware.c`) can only be tested on hardware. When changing
it, test on a real cartridge: start games with and without add-ons, from the
SD card and from NOR, and check saves and save states.

During the rewrite the patch engine was additionally compared with the
original kernel's patcher on 3,000 generated ROMs (all sizes, padding,
special-case games, cheats, PSRAM and NOR): the images were identical except
where the original had bugs.

## Code style

- C17 (`-std=gnu17`), formatted with clang-format 18 (`.clang-format`):
  4-space indent, 100 columns, Linux braces. Run `make format`.
- Names: `module_verb_object()` functions in `snake_case`, `_t` types,
  `UPPER_CASE` constants and macros, `g_` for the few globals, `s_` for
  file-local state.
- Every public function has a Doxygen comment in its header; files start with
  `@file` / `@brief`.
- Includes: own header first, then system headers, then project headers in
  alphabetical order, as paths from `src/` (`#include "core/text.h"`).
- No warnings: the build uses `-Wall -Wextra -Wshadow -Wstrict-prototypes
  -Wmissing-prototypes`, the tests add `-Werror`.
- Portable code goes in `core/` and gets unit tests. Hardware access goes in
  `hal/` and follows the [code placement rules](architecture.md#code-placement-rules).
- `make lint` runs cppcheck.

## Continuous integration

`.github/workflows/ci.yml` runs on every push and pull request:

| Job | Steps |
| --- | --- |
| Build kernel | `make` in the `devkitpro/devkitarm` container (including the IWRAM and Thumb checks), memory report, firmware CRC check, upload of `ezkernel.bin` |
| Host tests | `make test` with GCC and with Clang, screenshots as artifact |
| Style and static analysis | clang-format, cppcheck, fonts match their sources, Python tools compile |

## Pull requests

Use the pull request template. Describe how you tested, especially on
hardware for changes below the UI, and include screenshots of UI changes.
