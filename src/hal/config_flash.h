/**
 * @file config_flash.h
 * @brief Persistent storage in the S71 flash that also holds the kernel.
 *
 * Two flash blocks above the kernel image are used as small key/value stores:
 *
 * | Offset           | Contents                                  |
 * |------------------|-------------------------------------------|
 * | ::CONFIG_NOR_TABLE_OFFSET | Table of games written to NOR    |
 * | ::CONFIG_SETTINGS_OFFSET  | Settings words (see core/settings.h) |
 *
 * Writing erases the whole block first, so callers always write the complete
 * structure.
 */
#ifndef HAL_CONFIG_FLASH_H
#define HAL_CONFIG_FLASH_H

#include <gba_base.h>

/** Offset of the NOR game table inside the S71 flash. */
#define CONFIG_NOR_TABLE_OFFSET 0x7A0000u
/** Offset of the settings block inside the S71 flash. */
#define CONFIG_SETTINGS_OFFSET 0x7B0000u
/** Bytes written for the settings block. */
#define CONFIG_SETTINGS_BYTES 0x200u
/** Highest offset the kernel image may reach (it shares the chip). */
#define CONFIG_KERNEL_MAX_SIZE CONFIG_NOR_TABLE_OFFSET

/** Device ID of the S71 PL064 variant, which lacks write buffers. */
#define CONFIG_FLASH_ID_PL064 0x2202u

/** @brief Read the S71 device ID. */
u16 IWRAM_CODE config_flash_read_id(void);

/** @brief Read the halfword at byte @p offset. */
u16 IWRAM_CODE config_flash_read16(u32 offset);

/** @brief Copy @p count halfwords starting at byte @p offset into @p dst. */
void IWRAM_CODE config_flash_read(u32 offset, u16 *dst, u32 count);

/**
 * @brief Erase the block at @p offset and program @p size bytes from @p src.
 *
 * @p size must be a multiple of 32 bytes (one write buffer).
 */
void IWRAM_CODE config_flash_write_block(u32 offset, const void *src, u32 size);

#endif /* HAL_CONFIG_FLASH_H */
