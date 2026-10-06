/**
 * @file save_backup.c
 * @brief Save file backups (`/SAVER/<game>.bak`). See save_files.h.
 */
#include "loader/save_files.h"

#include "core/path.h"
#include "ff.h"
#include "loader/buffers.h"
#include "loader/sd_paths.h"

/** Largest save file (FLASH 1M); fits the scratch buffer. */
#define SAVE_MAX_SIZE 0x20000u

static uint32_t file_size(const char *path)
{
    FILINFO info;
    return f_stat(path, &info) == FR_OK ? (uint32_t)info.fsize : 0;
}

static bool backup_path(char *dst, size_t size, const char *game_filename)
{
    return sd_companion_path(dst, size, SD_DIR_SAVES, game_filename, "bak");
}

bool save_file_backup(const char *game_filename)
{
    char path[PATH_MAX_LEN];
    FIL file;
    UINT done = 0;

    if (!sd_companion_path(path, sizeof(path), SD_DIR_SAVES, game_filename, "sav") ||
        f_open(&file, path, FA_READ) != FR_OK) {
        return false;
    }
    uint32_t size = f_size(&file);
    bool ok = size > 0 && size <= SAVE_MAX_SIZE && f_read(&file, g_scratch, size, &done) == FR_OK &&
              done == size;
    f_close(&file);
    if (!ok) {
        return false;
    }

    /* A save that was never written is not worth keeping. */
    uint32_t i = 0;
    while (i < size && g_scratch[i] == 0xFF) {
        i++;
    }
    if (i == size) {
        return false;
    }

    if (!backup_path(path, sizeof(path), game_filename) ||
        f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return false;
    }
    ok = f_write(&file, g_scratch, size, &done) == FR_OK && done == size;
    f_close(&file);
    return ok;
}

bool save_backup_exists(const char *game_filename)
{
    char path[PATH_MAX_LEN];
    return backup_path(path, sizeof(path), game_filename) && file_size(path) > 0;
}

bool save_backup_restore(const char *game_filename)
{
    char save[PATH_MAX_LEN];
    char backup[PATH_MAX_LEN];
    static const char swap[] = SD_DIR_SAVES "/swap.tmp";

    if (!sd_companion_path(save, sizeof(save), SD_DIR_SAVES, game_filename, "sav") ||
        !backup_path(backup, sizeof(backup), game_filename) || file_size(backup) == 0) {
        return false;
    }
    f_unlink(swap);
    if (file_size(save) == 0) {
        f_unlink(save); /* an empty file */
        return f_rename(backup, save) == FR_OK;
    }
    /* Swap the two, so the restore can be undone the same way. */
    return f_rename(save, swap) == FR_OK && f_rename(backup, save) == FR_OK &&
           f_rename(swap, backup) == FR_OK;
}
