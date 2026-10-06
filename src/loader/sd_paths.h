/**
 * @file sd_paths.h
 * @brief The SD card file system, and its fixed folders and files
 * (see docs/sd-card-layout.md).
 */
#ifndef LOADER_SD_PATHS_H
#define LOADER_SD_PATHS_H

#include <stdbool.h>
#include <stddef.h>

#include "ff.h"

/** Save files, `.mde` files and the recently played list. */
#define SD_DIR_SAVES "/SAVER"
/** Save-state files (`.rts`). */
#define SD_DIR_SAVE_STATES "/RTS"
/** Patch caches (`.pat`). */
#define SD_DIR_PATCH_CACHE "/PATCH"
/** Cheat files and the cheat library. */
#define SD_DIR_CHEATS "/CHEAT"
/** Box-art thumbnails: /IMGS/<c0>/<c1>/<code>.bmp. */
#define SD_DIR_THUMBNAILS "/IMGS"

/** The recently played list. */
#define SD_FILE_RECENT SD_DIR_SAVES "/Recently play.txt"
/** Favorite games, one path per line. */
#define SD_FILE_FAVORITES SD_DIR_SAVES "/Favorites.txt"
/** Game code -> cheat library number. */
#define SD_FILE_CHEAT_INDEX SD_DIR_CHEATS "/GameID2cht.bin"

/** The mounted SD card file system (valid after sd_mount()). */
extern FATFS g_fs;

/** @brief Mount the SD card. @return true on success. */
bool sd_mount(void);

/**
 * @brief Build "<dir>/<companion name of filename with ext3>".
 * Example: ("/PATCH", "Game.gba", "pat") -> "/PATCH/Game.pat".
 */
bool sd_companion_path(char *dst, size_t size, const char *dir, const char *filename,
                       const char *ext3);

/**
 * @brief The full (long) name of the file at @p path, which may have been
 * opened by its 8.3 short name. Falls back to the last part of @p path.
 * Companion files (saves, save states, ...) are named after this name.
 */
void sd_long_name(const char *path, char *out, size_t size);

/** @brief Create a folder if it does not exist. @return true if it exists afterwards. */
bool sd_ensure_folder(const char *path);

#endif /* LOADER_SD_PATHS_H */
