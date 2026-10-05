/**
 * @file save_files.h
 * @brief Game saves, save states and the per-game save-type choice.
 */
#ifndef LOADER_SAVE_FILES_H
#define LOADER_SAVE_FILES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/save_type.h"

/** Size of a save-state file (all of the SRAM pages it maps). */
#define SAVE_STATE_FILE_SIZE 0x70000u

/**
 * @brief Absolute path of a game's save file in /SAVER.
 */
bool save_file_path(char *dst, size_t size, const char *game_filename);

/**
 * @brief Size of an existing save file, or 0 if it does not exist.
 */
uint32_t save_file_size(const char *path);

/** @brief Create a save file of @p size bytes filled with 0xFF. */
bool save_file_create(const char *path, uint32_t size);

/** @brief Copy a save file (up to 128 KiB) into cartridge SRAM. */
bool save_file_load_to_sram(const char *path);

/**
 * @brief Prepare the save-state file of a game: create it if needed, copy it
 * into SRAM and add it to the FAT map.
 * @param created Set to true if a new file had to be created.
 * @return false on any error.
 */
bool save_state_prepare(const char *game_filename, bool *created);

/** @brief The save type chosen for a game in the menu (stored in its `.mde` file). */
save_choice_t save_choice_read(const char *game_filename);

/** @brief Remember the save type chosen for a game. */
bool save_choice_write(const char *game_filename, save_choice_t choice);

#endif /* LOADER_SAVE_FILES_H */
