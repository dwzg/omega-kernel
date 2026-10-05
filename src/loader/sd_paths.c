/**
 * @file sd_paths.c
 * @brief SD card file system and path helpers. See sd_paths.h.
 */
#include "loader/sd_paths.h"

#include "core/game_file.h"
#include "core/path.h"
#include "ff.h"

FATFS g_fs;

bool sd_mount(void)
{
    return f_mount(&g_fs, "", 1) == FR_OK;
}

bool sd_companion_path(char *dst, size_t size, const char *dir, const char *filename,
                       const char *ext3)
{
    char name[128];
    return game_file_companion(name, sizeof(name), filename, ext3) &&
           path_join(dst, size, dir, name);
}

bool sd_ensure_folder(const char *path)
{
    FRESULT res = f_mkdir(path);
    return res == FR_OK || res == FR_EXIST;
}
