/**
 * @file fat_map.h
 * @brief The "FAT map": where on the SD card the FPGA finds the game, its
 * save file and its save-state file.
 *
 * The FPGA reads (and for saves, writes back) these files on its own while a
 * game runs, so it needs their physical location. The kernel sends it a
 * 1 KiB table (omega_send_fat_map()) with one *slot* per file. A slot is a
 * list of runs of consecutive sectors:
 *
 *     { file_sector_offset, card_sector } ... { 0xFFFFFFFF, 0 }
 *
 * Layout of the table (byte offsets):
 *
 * | Offset  | Contents                                             |
 * |---------|------------------------------------------------------|
 * | `0x000` | ROM runs (up to 62)                                   |
 * | `0x1F0` | ROM size in bytes                                     |
 * | `0x1F4` | 1 = FPGA copies the ROM into PSRAM, 2 = already there |
 * | `0x1F8` | Sectors per cluster                                   |
 * | `0x1FC` | Save mode (bits 24-31) and save size (bits 0-23)      |
 * | `0x200` | Save file runs (up to 32)                             |
 * | `0x300` | Save state (`.rts`) runs (up to 32)                   |
 */
#ifndef LOADER_FAT_MAP_H
#define LOADER_FAT_MAP_H

#include <stdbool.h>
#include <stdint.h>

#include "core/save_type.h"
#include "ff.h"

/** Which file a slot describes. */
typedef enum {
    FAT_MAP_ROM,
    FAT_MAP_SAVE,
    FAT_MAP_SAVE_STATE,
} fat_map_slot_t;

/** Value of the copy-mode word. */
typedef enum {
    FAT_MAP_FPGA_COPIES_ROM = 1,
    FAT_MAP_ROM_ALREADY_LOADED = 2,
} fat_map_copy_mode_t;

/** Result of fat_map_add_file(). */
typedef enum {
    FAT_MAP_OK,
    FAT_MAP_CANNOT_OPEN,
    FAT_MAP_TOO_FRAGMENTED,
} fat_map_result_t;

/** The table itself (32-bit aligned for DMA). */
extern uint32_t g_fat_map[0x400 / 4];

/** @brief Clear the table (all slots empty). */
void fat_map_reset(void);

/** @brief Describe @p path (relative to the current directory) in @p slot. */
fat_map_result_t fat_map_add_file(FATFS *fs, const char *path, fat_map_slot_t slot);

/** @brief true if the save slot holds at least one run on the card. */
bool fat_map_save_present(void);

/** @brief Fill in the parameter words at 0x1F0-0x1FC. */
void fat_map_set_parameters(uint32_t rom_size, fat_map_copy_mode_t copy_mode,
                            uint32_t sectors_per_cluster, save_mode_t save_mode,
                            uint32_t save_size);

/** @brief Change only the copy-mode word. */
void fat_map_set_copy_mode(fat_map_copy_mode_t copy_mode);

#endif /* LOADER_FAT_MAP_H */
