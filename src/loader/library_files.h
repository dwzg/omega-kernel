/**
 * @file library_files.h
 * @brief Small per-library files on the SD card: recently played list,
 * cheat files and box-art thumbnails.
 */
#ifndef LOADER_LIBRARY_FILES_H
#define LOADER_LIBRARY_FILES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/cheat.h"
#include "core/recent.h"
#include "ff.h"

/** Thumbnail size in pixels. */
#define THUMBNAIL_WIDTH 120
#define THUMBNAIL_HEIGHT 80

/** @brief Load `/SAVER/Recently play.txt`. */
void recent_file_load(recent_list_t *list);

/** @brief Write the list back to `/SAVER/Recently play.txt`. */
bool recent_file_save(const recent_list_t *list);

/**
 * @brief Find the cheat file for a game: `/CHEAT/<name>.cht`, or the shared
 * library entry listed in `/CHEAT/GameID2cht.bin`.
 * @param game_path Absolute path of the game.
 * @param out       Receives the absolute path of the cheat file.
 * @return false if the game has no cheat file.
 */
bool cheat_file_find(const char *game_path, char *out, size_t out_size);

/** A ::cht_reader_t over an open FatFs file. */
typedef struct {
    FIL file;
    cht_reader_t reader;
} cheat_file_t;

/** @brief Open a cheat file for parsing with the functions in cheat.h. */
bool cheat_file_open(cheat_file_t *cf, const char *path);

/** @brief Close a file opened with cheat_file_open(). */
void cheat_file_close(cheat_file_t *cf);

/**
 * @brief Load the thumbnail `/IMGS/<c>/<c>/<code>.bmp` of a game into
 * ::g_scratch.
 * @return Pointer to 120x80 15-bit pixels (top row first), or NULL.
 */
const uint16_t *thumbnail_load(const char game_code[4]);

#endif /* LOADER_LIBRARY_FILES_H */
