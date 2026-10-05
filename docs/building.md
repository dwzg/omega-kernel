# Building

## Requirements

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the `gba-dev`
  group (devkitARM, libgba, bin2s). Any current release works; CI uses
  the `devkitpro/devkitarm` container.
- GNU Make and Python 3 (both come with devkitPro's MSYS2 on Windows).

## Building

```sh
make -j8
```

Output:

| File | Use |
| --- | --- |
| `ezkernel.bin` | The upgrade file for the cartridge |
| `ezkernel.gba` | The same ROM, for emulators and debuggers |
| `ezkernel.elf` | With debug symbols |
| `build/ezkernel.map` | Linker map |

The build fails on any of these, so a broken ROM is never produced:

- static data larger than EWRAM or IWRAM (`tools/memory_limits.ld`);
- IWRAM code calling ROM code where that is unsafe (`tools/check_iwram_calls.py`);
- an assembly routine called from C that isn't marked as Thumb
  (`tools/check_thumb_entries.py`).

`make size` shows memory use, `make clean` removes all output, `V=1` shows the
compiler commands.

### Without devkitPro

```sh
tools/docker-build.sh -j8
```

runs the same build in the official devkitARM container (Docker required).

### Windows

Install devkitPro, open the *MSYS2* shell it installs, change to the
repository and run `make`.

## Versions

The version comes from the `VERSION` file, the revision from `git describe`;
both are shown under **About**. Override them with
`make VERSION=2.1.0-test REVISION=local`.

## What gets built

```
assets/fonts/*.bin     ─┐
assets/firmware/*.bin  ─┼─ bin2s ─► build/gen/*.s, *.h ─┐
assets/patches/*.bin   ─┘                               ├─► ezkernel.elf ─ gbafix ─► ezkernel.gba
src/**/*.c, *.s, third_party/fatfs/*.c ────────────────┘
```

Generated headers in `build/gen` declare `<name>_bin` and `<name>_bin_size` for
each asset. Our own code is compiled with `-Wall -Wextra -Wshadow
-Wstrict-prototypes -Wmissing-prototypes` and must stay free of warnings;
third-party code and generated tables with `-Wall` only.

## Other targets

| Target | Needs | Does |
| --- | --- | --- |
| `make test` | C compiler, Python 3, mtools, dosfstools | Unit tests and UI simulator (see [Development](development.md)) |
| `make format` / `format-check` | clang-format 18 | Format / check the sources |
| `make lint` | cppcheck | Static analysis |
| `make docs` | Doxygen | API documentation in `build/docs/html` |

These don't need devkitPro.
