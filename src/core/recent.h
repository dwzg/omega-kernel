/**
 * @file recent.h
 * @brief Most-recently-played list.
 *
 * Stored on the SD card as `/SAVER/Recently play.txt`, one absolute path per
 * line, newest first. This module only manages the list in memory; reading
 * and writing the file is done by loader/recent_file.c.
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_RECENT_H
#define CORE_RECENT_H

#include <stdbool.h>
#include <stddef.h>

#include "core/path.h"

/** Maximum number of remembered games. */
#define RECENT_MAX 10
/** Room for a directory path, a '/', and a file name. */
#define RECENT_ENTRY_LEN (PATH_MAX_LEN + 104)

/** The list, newest entry first. */
typedef struct {
    unsigned count;
    char entries[RECENT_MAX][RECENT_ENTRY_LEN];
} recent_list_t;

/** @brief Empty the list. */
void recent_clear(recent_list_t *list);

/**
 * @brief Append a line read from the file (oldest entries come last).
 * @return false if the line is not an absolute path or the list is full;
 *         parsing should stop at that point.
 */
bool recent_append_line(recent_list_t *list, const char *line);

/**
 * @brief Record that @p path was played: move it to the front, or insert it
 * and drop the oldest entry if the list is full.
 */
void recent_touch(recent_list_t *list, const char *path);

#endif /* CORE_RECENT_H */
