/**
 * @file nor.h
 * @brief S98 NOR flash (64 MiB) that stores games written with "Copy to NOR".
 *
 * The flash is seen through an 8 MiB window at ::NOR_BASE. The functions here
 * take *absolute* NOR offsets (0 .. ::NOR_TOTAL_SIZE) and switch the window
 * page as needed, restoring ::KERNEL_ROM_PAGE afterwards.
 *
 * Commands follow the usual AMD/Spansion command set (0xAA/0x55 unlock at
 * word addresses 0x555/0x2AA).
 */
#ifndef HAL_NOR_H
#define HAL_NOR_H

#include <gba_base.h>

/** Device ID returned by the S98 NOR flash. */
#define NOR_DEVICE_ID_S98 0x223Du

/** @brief Read the device ID (leaves the chip in read-array mode). */
u16 nor_read_id(void);

/** @brief Return the chip to read-array mode. */
void nor_reset(void);

/**
 * @brief Erase the 128 KiB block that starts at @p offset.
 *
 * The first and last blocks of the chip are made of four 32 KiB sectors;
 * all four are erased.
 */
void nor_erase_block(u32 offset);

/**
 * @brief Program @p size bytes at @p offset one halfword at a time.
 * @note Slow; kept for reference. nor_program_buffered() is used normally.
 */
void nor_program(u32 offset, const u8 *data, u32 size);

/** @brief Program @p size bytes (multiple of 32) at @p offset using 32-byte write buffers. */
void IWRAM_CODE nor_program_buffered(u32 offset, const u8 *data, u32 size);

/**
 * @brief Clear the persistent sector protection bits of the whole chip.
 *
 * Issued before writing a game so that previously protected sectors can be
 * erased. Takes a fixed polling time of roughly 0.5 s.
 */
void nor_unprotect_all(void);

/**
 * @brief Erase the whole chip.
 * @param poll Called repeatedly while the erase runs (it takes minutes); use
 *             it to wait for vblank and update a progress display. Called
 *             with an increasing counter.
 */
void nor_erase_chip(void (*poll)(u32 tick));

#endif /* HAL_NOR_H */
