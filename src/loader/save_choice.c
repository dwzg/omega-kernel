/**
 * @file save_choice.c
 * @brief Per-game save type choice (`.mde` files in /SAVER). See save_files.h.
 */
#include "loader/save_files.h"

#include "core/path.h"
#include "ff.h"
#include "loader/sd_paths.h"

/** Size of an `.mde` file; only the first byte is used. */
#define MDE_FILE_SIZE 16

save_choice_t save_choice_read(const char *game_filename)
{
    char path[PATH_MAX_LEN];
    FIL file;
    UINT read = 0;
    uint8_t data[MDE_FILE_SIZE] = {0};

    if (!sd_companion_path(path, sizeof(path), SD_DIR_SAVES, game_filename, "mde") ||
        f_open(&file, path, FA_READ) != FR_OK) {
        return SAVE_CHOICE_AUTO;
    }
    f_read(&file, data, sizeof(data), &read);
    f_close(&file);
    return (read > 0 && data[0] < SAVE_CHOICE_COUNT) ? (save_choice_t)data[0] : SAVE_CHOICE_AUTO;
}

bool save_choice_write(const char *game_filename, save_choice_t choice)
{
    char path[PATH_MAX_LEN];
    FIL file;
    UINT written;
    uint8_t data[MDE_FILE_SIZE] = {(uint8_t)choice};

    if (!sd_ensure_folder(SD_DIR_SAVES) ||
        !sd_companion_path(path, sizeof(path), SD_DIR_SAVES, game_filename, "mde") ||
        f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return false;
    }
    bool ok = f_write(&file, data, sizeof(data), &written) == FR_OK && written == sizeof(data);
    f_close(&file);
    return ok;
}
