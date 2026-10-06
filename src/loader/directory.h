/**
 * @file directory.h
 * @brief Listing a folder of the SD card for the file browser.
 *
 * Shows sub-folders first, then GBA games, each group sorted by name (see
 * text_compare_names()). Hidden and system entries and macOS resource files
 * ("._Name.gba") are skipped.
 *
 * Names are kept in one pool, with an 8-byte record per entry, so a folder
 * can hold up to ::DIR_MAX_ENTRIES entries and sorting moves only records.
 */
#ifndef LOADER_DIRECTORY_H
#define LOADER_DIRECTORY_H

#include <stdbool.h>
#include <stdint.h>

/** Space for a name shown in short lists (recently played, favorites). */
#define DIR_NAME_LEN 84
/** Maximum number of entries (folders and games) listed. */
#define DIR_MAX_ENTRIES 2048
/** Bytes for the names of one listing (about 30 per entry on average). */
#define DIR_NAME_POOL_SIZE 63232

/** Summary of the last listing. */
typedef struct {
    unsigned folders; /**< Entries 0 .. folders-1 are folders. */
    unsigned files;   /**< Entries folders .. folders+files-1 are games. */
    bool truncated;   /**< Some entries did not fit (too many, or names too long). */
} dir_listing_t;

/**
 * @brief Read the folder @p path (absolute).
 * @return false if the folder cannot be opened (the listing is then empty).
 */
bool directory_read(const char *path, dir_listing_t *listing);

/** @brief Full UTF-8 name of entry @p index of the last listing (folders first). */
const char *directory_name(unsigned index);

/** @brief Size in bytes of entry @p index (0 for folders). */
uint32_t directory_size(unsigned index);

/** @brief true if entry @p index of the last listing is a folder. */
bool directory_is_folder(unsigned index);

/**
 * @brief The name to open entry @p index with: its full name, or its 8.3
 * short name if the full name would make the path too long.
 */
const char *directory_open_name(unsigned index);

/** @brief Index of the entry opened as @p name in the last listing, or -1. */
int directory_find(const char *name);

/**
 * @brief Where to jump from entry @p index to reach the next (@p direction
 * > 0) or previous letter.
 *
 * Entries are grouped by their first character as sorted (ignoring case and
 * accents; all digits form one group), folders separately from files.
 * Forwards goes to the first entry of the next group; backwards to the
 * first entry of the current group, or of the previous one when already
 * there. Returns @p index when there is nowhere to go.
 */
unsigned directory_jump(unsigned index, int direction);

#endif /* LOADER_DIRECTORY_H */
