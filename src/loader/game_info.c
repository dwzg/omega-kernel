/**
 * @file game_info.c
 * @brief Game file header reading. See game_info.h.
 */
#include "loader/game_info.h"

#include <string.h>

#include "ff.h"

bool game_info_read(const char *path, game_info_t *info)
{
    FIL file;
    UINT read = 0;

    info->size = 0;
    memcpy(info->game_code, "FFFF", 5);

    if (f_open(&file, path, FA_READ) != FR_OK) {
        return false;
    }
    info->size = f_size(&file);
    if (f_lseek(&file, ROM_HEADER_GAME_CODE) == FR_OK) {
        f_read(&file, info->game_code, 4, &read);
    }
    f_close(&file);
    if (read != 4) {
        memcpy(info->game_code, "FFFF", 5);
    }
    info->game_code[4] = '\0';
    return true;
}
