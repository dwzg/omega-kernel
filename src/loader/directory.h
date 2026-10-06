/**
 * @file directory.h
 * @brief Listing a folder of the SD card for the file browser.
 *
 * Shows sub-folders first, then GBA games, each group sorted by name
 * (ignoring case). Hidden and system entries and macOS resource files
 * ("._Name.gba") are skipped.
 */
#ifndef LOADER_DIRECTORY_H
#define LOADER_DIRECTORY_H

#include <stdbool.h>
#include <stdint.h>

/** Space for a displayed name; longer names are shortened for display. */
#define DIR_NAME_LEN 84
/** Maximum number of folders listed. */
#define DIR_MAX_FOLDERS 256
/** Maximum number of games listed. */
#define DIR_MAX_FILES 512

/** One listed entry. */
typedef struct {
    char name[DIR_NAME_LEN]; /**< UTF-8 name, cut short if it doesn't fit. */
    char short_name[13];     /**< 8.3 name if @ref name was cut short, else "". */
    uint32_t size;           /**< Bytes (0 for folders). */
} dir_entry_t;

/** Summary of the last listing. */
typedef struct {
    unsigned folders; /**< Entries 0 .. folders-1 are folders. */
    unsigned files;   /**< Entries folders .. folders+files-1 are games. */
    bool truncated;   /**< Some entries did not fit. */
} dir_listing_t;

/**
 * @brief Read the folder @p path (absolute).
 * @return false if the folder cannot be opened (the listing is then empty).
 */
bool directory_read(const char *path, dir_listing_t *listing);

/** @brief Entry @p index of the last listing (folders first). */
const dir_entry_t *directory_entry(unsigned index);

/** @brief true if entry @p index of the last listing is a folder. */
bool directory_is_folder(unsigned index);

/**
 * @brief The name to open entry @p index with: its full name, or its 8.3
 * short name if the full name was too long to keep.
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
