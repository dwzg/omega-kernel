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
#include "loader/progress.h"

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

/**
 * @brief Copy a game's save file to `/SAVER/<game>.bak`, replacing the
 * previous backup. A save that is still blank (all 0xFF) is not copied.
 * @return true if a backup was written.
 */
bool save_file_backup(const char *game_filename);

/** @brief Whether a game has a save backup. */
bool save_backup_exists(const char *game_filename);

/**
 * @brief Swap a game's save file and its backup (so calling it again undoes
 * it). Without a save file, the backup becomes the save file.
 */
bool save_backup_restore(const char *game_filename);

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

/**
 * @brief The save type found earlier by scanning a game (cached in its
 * `.mde` file), if it was made for this game code and ROM size.
 */
bool save_detected_read(const char *game_filename, const char game_code[4], uint32_t rom_size,
                        save_mode_t *mode);

/** @brief Cache the save type found by scanning a game. */
bool save_detected_write(const char *game_filename, const char game_code[4], uint32_t rom_size,
                         save_mode_t mode);

/**
 * @brief The save mode to use for a game.
 *
 * A forced choice or the database decide first. A game missing from the
 * database is scanned for its save library's marker (see
 * save_type_detect()) the first time it is started; the result is cached.
 *
 * @param rom_path  The game file to scan, or NULL to only use the cache.
 * @param progress  Shows the scan; may be NULL.
 */
save_mode_t save_mode_for_game(save_choice_t choice, const char *game_filename,
                               const char game_code[4], uint32_t rom_size, const char *rom_path,
                               const progress_t *progress);

#endif /* LOADER_SAVE_FILES_H */
