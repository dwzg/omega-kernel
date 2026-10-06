/**
 * @file directory.c
 * @brief Folder listing. See directory.h.
 */
#include "loader/directory.h"

#include <stdlib.h>
#include <string.h>

#include "core/game_file.h"
#include "core/path.h"
#include "core/text.h"
#include "core/utf8.h"
#include "ff.h"
#include "platform/attributes.h"

/** One listed entry; names are offsets into ::s_pool. */
typedef struct {
    uint32_t size; /**< Bytes; ::FOLDER_FLAG marks a folder. */
    uint16_t name; /**< Full name. */
    uint16_t open; /**< Name to open it by (== name, or the 8.3 name). */
} record_t;

#define FOLDER_FLAG 0x80000000u

_Static_assert(DIR_NAME_POOL_SIZE <= 0x10000, "pool offsets are 16 bits");

static record_t s_records[DIR_MAX_ENTRIES] PLATFORM_EWRAM;
static char s_pool[DIR_NAME_POOL_SIZE] PLATFORM_EWRAM;
static unsigned s_pool_used;
static dir_listing_t s_listing;

/** Store @p name in the pool. @return its offset, or -1 if it is full. */
static int pool_add(const char *name)
{
    size_t len = strlen(name) + 1;
    if (s_pool_used + len > sizeof(s_pool)) {
        return -1;
    }
    memcpy(s_pool + s_pool_used, name, len);
    s_pool_used += (unsigned)len;
    return (int)(s_pool_used - len);
}

/** Folders first, then by name. */
static int compare_records(const void *a, const void *b)
{
    const record_t *ra = a;
    const record_t *rb = b;
    bool fa = (ra->size & FOLDER_FLAG) != 0;
    bool fb = (rb->size & FOLDER_FLAG) != 0;
    if (fa != fb) {
        return fa ? -1 : 1;
    }
    return text_compare_names(s_pool + ra->name, s_pool + rb->name);
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
    unsigned count = 0;
    size_t path_len = strlen(path);

    memset(&s_listing, 0, sizeof(s_listing));
    s_pool_used = 0;
    if (f_opendir(&dir, path) != FR_OK) {
        *listing = s_listing;
        return false;
    }
    while (f_readdir(&dir, &info) == FR_OK && info.fname[0] != '\0') {
        if (!is_listed(&info)) {
            continue;
        }
        if (count >= DIR_MAX_ENTRIES) {
            s_listing.truncated = true;
            continue;
        }
        record_t *r = &s_records[count];
        int name = pool_add(info.fname);
        int open = name;
        /* A path too long to handle opens through the 8.3 short name. */
        if (name >= 0 && path_len + 1 + strlen(info.fname) >= PATH_MAX_LEN && info.altname[0]) {
            open = pool_add(info.altname);
        }
        if (name < 0 || open < 0) {
            s_pool_used = name < 0 ? s_pool_used : (unsigned)name; /* drop it */
            s_listing.truncated = true;
            continue;
        }
        bool folder = (info.fattrib & AM_DIR) != 0;
        r->name = (uint16_t)name;
        r->open = (uint16_t)open;
        r->size = folder ? FOLDER_FLAG : (uint32_t)info.fsize & ~FOLDER_FLAG;
        if (folder) {
            s_listing.folders++;
        } else {
            s_listing.files++;
        }
        count++;
    }
    f_closedir(&dir);

    qsort(s_records, count, sizeof(s_records[0]), compare_records);
    *listing = s_listing;
    return true;
}

static const record_t *record(unsigned index)
{
    return index < s_listing.folders + s_listing.files ? &s_records[index] : NULL;
}

const char *directory_name(unsigned index)
{
    const record_t *r = record(index);
    return r ? s_pool + r->name : "";
}

const char *directory_open_name(unsigned index)
{
    const record_t *r = record(index);
    return r ? s_pool + r->open : "";
}

uint32_t directory_size(unsigned index)
{
    const record_t *r = record(index);
    return r ? r->size & ~FOLDER_FLAG : 0;
}

int directory_find(const char *name)
{
    for (unsigned i = 0; i < s_listing.folders + s_listing.files; i++) {
        if (strcmp(directory_open_name(i), name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/** The group of entry @p index for directory_jump(). */
static uint32_t group_of(unsigned index)
{
    const char *name = directory_name(index);
    uint32_t c = text_fold(utf8_next(&name));
    if (c >= '0' && c <= '9') {
        c = '0';
    }
    return directory_is_folder(index) ? c | 0x80000000u : c;
}

unsigned directory_jump(unsigned index, int direction)
{
    unsigned total = s_listing.folders + s_listing.files;
    if (index >= total) {
        return index;
    }
    uint32_t group = group_of(index);
    if (direction > 0) {
        unsigned i = index + 1;
        while (i < total && group_of(i) == group) {
            i++;
        }
        return i < total ? i : index;
    }
    unsigned i = index;
    if (i > 0 && group_of(i - 1) != group) {
        group = group_of(--i); /* at a group start: go to the previous one */
    }
    while (i > 0 && group_of(i - 1) == group) {
        i--;
    }
    return i;
}

bool directory_is_folder(unsigned index)
{
    return index < s_listing.folders;
}
