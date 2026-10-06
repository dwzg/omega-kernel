/**
 * @file favorites.h
 * @brief The list of favorite games.
 *
 * Stored on the SD card as `/SAVER/Favorites.txt`, one absolute path per
 * line. This module only manages the list in memory; reading and writing
 * the file is done by loader/library_files.c.
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_FAVORITES_H
#define CORE_FAVORITES_H

#include <stdbool.h>

#include "core/recent.h"

/** Maximum number of favorites. */
#define FAVORITES_MAX 64

typedef struct {
    unsigned count;
    char entries[FAVORITES_MAX][RECENT_ENTRY_LEN];
} favorites_t;

/** @brief Empty the list. */
void favorites_clear(favorites_t *list);

/**
 * @brief Add a line read from the file. Lines that are not absolute paths
 * are skipped. @return false when the list is full.
 */
bool favorites_append_line(favorites_t *list, const char *line);

/** @brief Index of @p path in the list, or -1. */
int favorites_find(const favorites_t *list, const char *path);

/**
 * @brief Add @p path if it is not in the list yet.
 * @return false if the list is full or the path too long.
 */
bool favorites_add(favorites_t *list, const char *path);

/** @brief Remove @p path if it is in the list. */
void favorites_remove(favorites_t *list, const char *path);

#endif /* CORE_FAVORITES_H */
