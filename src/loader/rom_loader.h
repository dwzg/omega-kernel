/**
 * @file rom_loader.h
 * @brief Copying games into PSRAM by software.
 *
 * Normally the FPGA copies a GBA ROM into PSRAM by itself (see fat_map.h).
 * The kernel copies it instead when the ROM has to be scanned for IRQ
 * references on the way (see patch.h).
 */
#ifndef LOADER_ROM_LOADER_H
#define LOADER_ROM_LOADER_H

#include <stdbool.h>
#include <stdint.h>

#include "loader/progress.h"
#include "patch/patch.h"

/**
 * @brief Copy a GBA ROM into PSRAM.
 * @param scan If not NULL, every block is scanned for IRQ references on the way.
 */
bool rom_load_to_psram(const char *path, patch_context_t *scan, const progress_t *progress);

/** @brief Read the 128 KiB block that contains the ROM's last byte into ::g_scratch. */
bool rom_read_last_block(const char *path, uint32_t rom_size);

#endif /* LOADER_ROM_LOADER_H */
