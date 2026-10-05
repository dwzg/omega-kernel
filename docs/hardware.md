# The EZ-FLASH OMEGA hardware

What the kernel knows about the cartridge. The vendor's own description is in
[`hardware/EZ-FLASH OMEGA DOCUMENT.pdf`](hardware/); the code is in `src/hal/`.

## Components

| Part | Size | Use |
| --- | --- | --- |
| FPGA | | Maps the memories into the GBA cartridge bus, talks to the SD card, saves automatically, provides save states and the in-game menu |
| S71 flash | | This kernel (from `0x08000000`), the NOR game table (`0x7A0000`) and settings (`0x7B0000`) |
| S98 NOR flash | 64 MB | Games copied with "Copy to NOR" |
| PSRAM | 32 MB | Games loaded from the SD card |
| SRAM | 64 KB windows, several banks | The running game's save memory, and save-state storage |
| RTC | | Real-time clock, GPIO-compatible with the one in Pokémon cartridges |
| microSD slot | | FAT32/exFAT card |

## Memory map while the kernel runs

| Address | Contents |
| --- | --- |
| `0x08000000` | S71 flash: the kernel |
| `0x08800000` | PSRAM window, 8 MB, paged by the PSRAM page register |
| `0x09000000` | NOR window, 8 MB, paged by the ROM page register |
| `0x09E00000` | FPGA data port |
| `0x0E000000` | SRAM, 64 KB, paged by the SRAM page register |

## FPGA registers

Registers are addresses in the ROM area. A write only takes effect inside
this unlock sequence (`omega_write_register()`):

```c
*(vu16 *)0x09FE0000 = 0xD200;
*(vu16 *)0x08000000 = 0x1500;
*(vu16 *)0x08020000 = 0xD200;
*(vu16 *)0x08040000 = 0x1500;
*(vu16 *)REGISTER   = value;
*(vu16 *)0x09FC0000 = 0x1500;
```

The sequence briefly changes what the ROM bus returns, so it must run from
IWRAM.

| Register | Name | Values |
| --- | --- | --- |
| `0x09400000` | SD control | 0 off, 1 on, 3 read status |
| `0x09420000` | Buffer control | access to the 1 KB FAT map buffer: 1 open, 3 copy ROM, 0 close |
| `0x09600000` | SD address low | sector bits 0-15 |
| `0x09620000` | SD address high | sector bits 16-31 |
| `0x09640000` | SD count | sector count; bit 15 set for writes |
| `0x09660000` | SPI control | 1 = data port returns the FPGA version / SPI data |
| `0x09680000` | SPI write | 1 = allow programming the FPGA configuration flash |
| `0x096A0000` | RTC | 1 = games see the RTC |
| `0x096C0000` | Auto-save | 1 = the FPGA writes SRAM changes back to the save file |
| `0x09860000` | PSRAM page | which 8 MB of PSRAM appears at `0x08800000` |
| `0x09880000` | ROM page | which memory appears at `0x08000000` |
| `0x09C00000` | SRAM page | which 64 KB SRAM bank appears at `0x0E000000` |
| `0x09E00000` | Data port | read: SD data and status, SPI data |

### ROM page values

| Value | Maps |
| --- | --- |
| `0x8002` | The kernel at `0x08000000`, NOR at `0x09000000` (kernel mode) |
| `0x8002 + 0x1000 × n` | NOR 8 MB window *n* at `0x09000000` |
| `0x0200` | PSRAM at `0x08000000` (a loaded game) |
| `offset >> 17` | A game in NOR starting at that offset |

### SRAM pages

| Page | Use |
| --- | --- |
| `0x00`, `0x10` | The game's save (two 64 KB banks for 128 KB flash saves) |
| `0x40`-`0xA0` | Save-state storage (7 banks = 448 KB) |

## SD card access

The FPGA does the SD protocol. The kernel (`src/hal/sd.c`) selects SD mode,
writes the sector address and count, waits until the status port stops
returning the busy value `0xEEE1`, then reads 512 bytes per sector from the
data port. Transfers are split into chunks of four sectors. A failed read is
retried once; after that the error is reported.

## NOR flash

Standard AMD-style command sequences (`src/hal/nor.c`): 128 KB erase blocks
(the first and last block are split into four 32 KB sectors each),
buffered programming, and chip erase. Sector protection bits are cleared
before writing. The chip is reached through the 8 MB NOR window and the ROM
page register.

## Configuration flash (S71)

Two chip variants exist; the kernel reads the chip ID and uses word-by-word
programming for the PL064 (`0x2202`) and 32-byte buffered programming
otherwise (`src/hal/config_flash.c`). Writing always erases the whole 64 KB
block first.

## Firmware

The FPGA loads its configuration from an SPI flash. Its version is read with
the SPI control register. The kernel embeds the newest image
(`assets/firmware/omega_fpga.bin`, version 9, CRC-32 `0xB23F6EAE`) and writes
it in 256-byte pages through the FAT map buffer (`src/loader/firmware.c`).

## Starting a game

`omega_boot()` sets the RTC register, maps the game with the ROM page
register, clears RAM with the BIOS (`RegisterRamReset`) and resets into the
game, either directly (`SoftReset` after clearing IWRAM) or through the BIOS
start-up (`HardReset`). See [Boot process](boot-process.md).
