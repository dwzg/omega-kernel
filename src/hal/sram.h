/**
 * @file sram.h
 * @brief Cartridge SRAM access (64 KiB window at ::SRAM_BASE, paged).
 *
 * SRAM must be accessed with 8-bit reads and writes.
 *
 * Page layout used by the kernel (omega_set_sram_page()):
 *
 * | Page          | Use                                                  |
 * |---------------|------------------------------------------------------|
 * | `0x00`, `0x10`| Game save data (128 KiB for FLASH1M games)            |
 * | `0x40`-`0xA0` | Real-time-save state (`.rts` file, 448 KiB)           |
 */
#ifndef HAL_SRAM_H
#define HAL_SRAM_H

#include <gba_base.h>

/** Size of the SRAM window. */
#define SRAM_WINDOW_SIZE 0x10000u
/** SRAM page holding the first 64 KiB of game save data. */
#define SRAM_PAGE_SAVE_LOW 0x00u
/** SRAM page holding the second 64 KiB of game save data. */
#define SRAM_PAGE_SAVE_HIGH 0x10u
/** First SRAM page of the real-time-save state. */
#define SRAM_PAGE_RTS_FIRST 0x40u
/** One past the last SRAM page of the real-time-save state. */
#define SRAM_PAGE_RTS_END 0xB0u
/** Distance between consecutive 64 KiB SRAM pages. */
#define SRAM_PAGE_STEP 0x10u

/** @brief Copy @p size bytes from SRAM @p address to @p data. */
void IWRAM_CODE sram_read(u32 address, u8 *data, u32 size);

/** @brief Copy @p size bytes from @p data to SRAM @p address. */
void IWRAM_CODE sram_write(u32 address, const u8 *data, u32 size);

/**
 * @brief Select a 64 KiB bank of an emulated FLASH1M save chip.
 *
 * Sends the standard Macronix/Sanyo bank-switch command sequence.
 */
void IWRAM_CODE sram_flash_bank_switch(u8 bank);

#endif /* HAL_SRAM_H */
