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

/** Longest name kept (longer names are cut, which only affects display). */
#define DIR_NAME_LEN 100
/** Maximum number of folders listed. */
#define DIR_MAX_FOLDERS 256
/** Maximum number of games listed. */
#define DIR_MAX_FILES 512

/** One listed entry. */
typedef struct {
    char name[DIR_NAME_LEN];
    uint32_t size; /**< Bytes (0 for folders). */
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

#endif /* LOADER_DIRECTORY_H */
