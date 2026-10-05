/**
 * @file directory.c
 * @brief Folder listing. See directory.h.
 */
#include "loader/directory.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "core/game_file.h"
#include "core/text.h"
#include "ff.h"
#include "platform/attributes.h"

static dir_entry_t s_folders[DIR_MAX_FOLDERS] PLATFORM_EWRAM;
static dir_entry_t s_files[DIR_MAX_FILES] PLATFORM_EWRAM;
static dir_listing_t s_listing;

static int compare_names(const void *a, const void *b)
{
    return strcasecmp(((const dir_entry_t *)a)->name, ((const dir_entry_t *)b)->name);
}

static bool is_listed(const FILINFO *info)
{
    if (info->fattrib & (AM_HID | AM_SYS)) {
        return false;
    }
    if (info->fname[0] == '.') {
        return false; /* ".", "..", "._Name.gba" and other dot files */
    }
    return (info->fattrib & AM_DIR) || game_file_is_gba(info->fname);
}

bool directory_read(const char *path, dir_listing_t *listing)
{
    DIR dir;
    FILINFO info;

    memset(&s_listing, 0, sizeof(s_listing));
    if (f_opendir(&dir, path) != FR_OK) {
        *listing = s_listing;
        return false;
    }
    while (f_readdir(&dir, &info) == FR_OK && info.fname[0] != '\0') {
        if (!is_listed(&info)) {
            continue;
        }
        bool folder = (info.fattrib & AM_DIR) != 0;
        unsigned *count = folder ? &s_listing.folders : &s_listing.files;
        unsigned limit = folder ? DIR_MAX_FOLDERS : DIR_MAX_FILES;
        if (*count >= limit) {
            s_listing.truncated = true;
            continue;
        }
        dir_entry_t *e = folder ? &s_folders[*count] : &s_files[*count];
        text_copy(e->name, sizeof(e->name), info.fname);
        e->size = folder ? 0 : (uint32_t)info.fsize;
        (*count)++;
    }
    f_closedir(&dir);

    /* Sorted in place: an index table would cost EWRAM, which is scarce. */
    qsort(s_folders, s_listing.folders, sizeof(s_folders[0]), compare_names);
    qsort(s_files, s_listing.files, sizeof(s_files[0]), compare_names);
    *listing = s_listing;
    return true;
}

bool directory_is_folder(unsigned index)
{
    return index < s_listing.folders;
}

const dir_entry_t *directory_entry(unsigned index)
{
    if (index < s_listing.folders) {
        return &s_folders[index];
    }
    index -= s_listing.folders;
    return index < s_listing.files ? &s_files[index] : NULL;
}
