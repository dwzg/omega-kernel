/**
 * @file sd_paths.c
 * @brief SD card file system and path helpers. See sd_paths.h.
 */
#include "loader/sd_paths.h"

#include <string.h>

#include "core/game_file.h"
#include "core/path.h"
#include "core/text.h"
#include "ff.h"

FATFS g_fs;

bool sd_mount(void)
{
    return f_mount(&g_fs, "", 1) == FR_OK;
}

bool sd_companion_path(char *dst, size_t size, const char *dir, const char *filename,
                       const char *ext3)
{
    char name[FF_LFN_BUF + 1];
    return game_file_companion(name, sizeof(name), filename, ext3) &&
           path_join(dst, size, dir, name);
}

/** Case-insensitive comparison of ASCII (8.3) names. */
static bool same_short_name(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        char ca = (*a >= 'a' && *a <= 'z') ? (char)(*a - 32) : *a;
        char cb = (*b >= 'a' && *b <= 'z') ? (char)(*b - 32) : *b;
        if (ca != cb) {
            return false;
        }
    }
    return *a == *b;
}

void sd_long_name(const char *path, char *out, size_t size)
{
    const char *base = path_basename(path);
    text_copy(out, size, base);
    if (!strchr(base, '~')) {
        return;
    }
    /* Looked up by its short name, FatFs reports the short name, so find
     * the entry in its folder to get the long one. */
    char dir_path[PATH_MAX_LEN];
    char name[13];
    DIR dir;
    FILINFO info;
    if (!path_split(path, dir_path, sizeof(dir_path), name, sizeof(name)) ||
        f_opendir(&dir, dir_path[0] ? dir_path : "/") != FR_OK) {
        return;
    }
    while (f_readdir(&dir, &info) == FR_OK && info.fname[0]) {
        if (same_short_name(info.altname, name)) {
            text_copy(out, size, info.fname);
            break;
        }
    }
    f_closedir(&dir);
}

bool sd_ensure_folder(const char *path)
{
    FRESULT res = f_mkdir(path);
    return res == FR_OK || res == FR_EXIST;
}
