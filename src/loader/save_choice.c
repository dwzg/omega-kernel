/**
 * @file save_choice.c
 * @brief Per-game save type choice (`.mde` files in /SAVER). See save_files.h.
 *
 * `.mde` layout (16 bytes; the original kernel uses only byte 0):
 *
 * | Byte  | Meaning                                                   |
 * | ----- | --------------------------------------------------------- |
 * | 0     | ::save_choice_t                                           |
 * | 1     | 'D' if bytes 2-11 hold a detected save type               |
 * | 2     | detected ::save_mode_t                                    |
 * | 4-7   | game code the detection was made for                      |
 * | 8-11  | ROM size it was made for (little-endian)                  |
 */
#include "loader/save_files.h"

#include <string.h>

#include "core/path.h"
#include "ff.h"
#include "loader/buffers.h"
#include "loader/sd_paths.h"

/** Size of an `.mde` file. */
#define MDE_FILE_SIZE 16
#define MDE_DETECTED 'D'

static bool mde_read(const char *game_filename, uint8_t data[MDE_FILE_SIZE])
{
    char path[PATH_MAX_LEN];
    FIL file;
    UINT read = 0;

    memset(data, 0, MDE_FILE_SIZE);
    if (!sd_companion_path(path, sizeof(path), SD_DIR_SAVES, game_filename, "mde") ||
        f_open(&file, path, FA_READ) != FR_OK) {
        return false;
    }
    f_read(&file, data, MDE_FILE_SIZE, &read);
    f_close(&file);
    return read > 0;
}

static bool mde_write(const char *game_filename, const uint8_t data[MDE_FILE_SIZE])
{
    char path[PATH_MAX_LEN];
    FIL file;
    UINT written;

    if (!sd_ensure_folder(SD_DIR_SAVES) ||
        !sd_companion_path(path, sizeof(path), SD_DIR_SAVES, game_filename, "mde") ||
        f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return false;
    }
    bool ok = f_write(&file, data, MDE_FILE_SIZE, &written) == FR_OK && written == MDE_FILE_SIZE;
    f_close(&file);
    return ok;
}

save_choice_t save_choice_read(const char *game_filename)
{
    uint8_t data[MDE_FILE_SIZE];
    mde_read(game_filename, data);
    return data[0] < SAVE_CHOICE_COUNT ? (save_choice_t)data[0] : SAVE_CHOICE_AUTO;
}

bool save_choice_write(const char *game_filename, save_choice_t choice)
{
    uint8_t data[MDE_FILE_SIZE];
    mde_read(game_filename, data); /* keep the detected type */
    data[0] = (uint8_t)choice;
    return mde_write(game_filename, data);
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

bool save_detected_read(const char *game_filename, const char game_code[4], uint32_t rom_size,
                        save_mode_t *mode)
{
    uint8_t data[MDE_FILE_SIZE];
    uint8_t size[4];
    put32(size, rom_size);
    if (!mde_read(game_filename, data) || data[1] != MDE_DETECTED ||
        memcmp(data + 4, game_code, 4) != 0 || memcmp(data + 8, size, 4) != 0) {
        return false;
    }
    *mode = (save_mode_t)data[2];
    return true;
}

bool save_detected_write(const char *game_filename, const char game_code[4], uint32_t rom_size,
                         save_mode_t mode)
{
    uint8_t data[MDE_FILE_SIZE];
    mde_read(game_filename, data);
    if (data[0] >= SAVE_CHOICE_COUNT) {
        data[0] = SAVE_CHOICE_AUTO;
    }
    data[1] = MDE_DETECTED;
    data[2] = (uint8_t)mode;
    memcpy(data + 4, game_code, 4);
    put32(data + 8, rom_size);
    return mde_write(game_filename, data);
}

save_mode_t save_mode_for_game(save_choice_t choice, const char *game_filename,
                               const char game_code[4], uint32_t rom_size, const char *rom_path,
                               const progress_t *progress)
{
    save_mode_t mode;
    if (choice != SAVE_CHOICE_AUTO || save_type_known(game_code)) {
        return save_type_resolve(choice, game_code, rom_size);
    }
    if (save_detected_read(game_filename, game_code, rom_size, &mode)) {
        return mode;
    }
    if (!rom_path) {
        return SAVE_MODE_DEFAULT;
    }

    /* Not in the database: look for the save library's marker, once. */
    FIL file;
    if (f_open(&file, rom_path, FA_READ) != FR_OK) {
        return SAVE_MODE_DEFAULT;
    }
    progress_status(progress, "Detecting save type");
    mode = SAVE_MODE_DEFAULT;
    uint32_t step = SCRATCH_SIZE - SAVE_TYPE_MARKER_OVERLAP;
    for (uint32_t offset = 0; offset < rom_size && mode == SAVE_MODE_DEFAULT; offset += step) {
        UINT read = 0;
        progress_advance(progress, offset, rom_size);
        if (f_lseek(&file, offset) != FR_OK ||
            f_read(&file, g_scratch, SCRATCH_SIZE, &read) != FR_OK || read == 0) {
            break;
        }
        mode = save_type_detect((const uint32_t *)g_scratch, read / 4, rom_size);
    }
    f_close(&file);
    save_detected_write(game_filename, game_code, rom_size, mode);
    return mode;
}
