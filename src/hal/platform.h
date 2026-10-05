/**
 * @file platform.h
 * @brief Memory map of the GBA + EZ-FLASH OMEGA and common platform macros.
 *
 * The OMEGA cartridge exposes several memories through the GBA cartridge bus.
 * Which memory is visible in a given address window is controlled by the
 * FPGA registers described in omega.h.
 *
 * | Address              | Contents while the kernel runs                         |
 * |----------------------|--------------------------------------------------------|
 * | `0x08000000`         | S71 flash: this kernel, settings and NOR game table     |
 * | `0x08800000`         | PSRAM window (8 MiB, paged with omega_set_psram_page()) |
 * | `0x09000000`         | S98 NOR flash window (8 MiB, paged with omega_set_rom_page()) |
 * | `0x09E00000`         | FPGA data port (SD data, responses, SPI)               |
 * | `0x0E000000`         | SRAM (64 KiB, paged with omega_set_sram_page())         |
 */
#ifndef HAL_PLATFORM_H
#define HAL_PLATFORM_H

#include <gba_base.h>

/** Base of the S71 flash that holds the kernel itself. */
#define FLASH_S71_BASE 0x08000000u

/** Base of the PSRAM window (games are copied here before booting). */
#define PSRAM_BASE 0x08800000u

/** Size of one PSRAM / NOR page window. */
#define ROM_WINDOW_SIZE 0x00800000u

/** Page register increment for each @ref ROM_WINDOW_SIZE step. */
#define ROM_WINDOW_PAGE_STEP 0x1000u

/** Base of the S98 NOR flash window (games written to NOR). */
#define NOR_BASE 0x09000000u

/** End of the S98 NOR flash window. */
#define NOR_WINDOW_END 0x09800000u

/** Total size of the S98 NOR flash (64 MiB). */
#define NOR_TOTAL_SIZE 0x04000000u

/** Erase block size of the S98 NOR flash. */
#define NOR_BLOCK_SIZE 0x00020000u

/** Base of cartridge SRAM. */
#define SRAM_BASE 0x0E000000u

/** Mode 3 frame buffer. */
#define VRAM_BASE ((u16 *)0x06000000)

/**
 * Unused VRAM above the mode 3 frame buffer. Patch payloads are assembled
 * here before they are written into the game image.
 */
#define VRAM_WORK_BUFFER 0x06012C00u

/** End of the VRAM work buffer (end of object VRAM). */
#define VRAM_WORK_BUFFER_END 0x06018000u

/**
 * ROM page of the kernel itself. NOR pages are addressed relative to this
 * value (see nor.c).
 */
#define KERNEL_ROM_PAGE 0x8002u

/** ROM page that maps PSRAM to the start of the cartridge space. */
#define PSRAM_ROM_PAGE 0x0200u

/**
 * Force inlining. Used for helpers called from ::IWRAM_CODE functions that
 * must never be emitted out of line into ROM (ROM may be unreadable while
 * they run).
 */
#define ALWAYS_INLINE inline __attribute__((always_inline))

/** Screen size in pixels. */
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

#endif /* HAL_PLATFORM_H */
