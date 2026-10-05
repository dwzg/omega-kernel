# FatFs R0.15

Generic FAT/exFAT file system module by ChaN — <http://elm-chan.org/fsw/ff/>.
The license is in [`00readme.txt`](00readme.txt) (BSD-style, one clause).

## Local changes

The files are upstream R0.15 except for:

| File | Change |
| --- | --- |
| `ffconf.h` | Project configuration: long file names (`FF_USE_LFN 1`), exFAT, relative paths (`FF_FS_RPATH 2`), `f_gets`/`f_printf` (`FF_USE_STRFUNC 1`), code page 437. |
| `ff.c` | Two exported wrappers at the end of the file, `Get_NextCluster()` and `ClustToSect()`, which expose the internal `get_fat()` and `clst2sect()` functions. The kernel uses them to build the sector map of a file that the cartridge FPGA reads by itself (see `src/loader/fat_map.c`). |

The disk I/O glue (`disk_read()`, `disk_write()`, `get_fattime()`, …) is
project code and lives in [`src/hal/diskio.c`](../../src/hal/diskio.c).

Please keep this directory as close to upstream as possible so it can be
updated by dropping in a newer release and re-applying the changes above.
