/**
 * @file save_files.c
 * @brief Save files, save states and `.mde` files. See save_files.h.
 */
#include "loader/save_files.h"

#include <string.h>

#include "core/path.h"
#include "ff.h"
#include "hal/omega.h"
#include "hal/platform.h"
#include "hal/sram.h"
#include "loader/buffers.h"
#include "loader/fat_map.h"
#include "loader/sd_paths.h"

/** The file system object (owned by main.c). */

/** Largest save file (FLASH 1M). */
#define SAVE_MAX_SIZE 0x20000u

bool save_file_path(char *dst, size_t size, const char *game_filename)
{
    return sd_companion_path(dst, size, SD_DIR_SAVES, game_filename, "sav");
}

uint32_t save_file_size(const char *path)
{
    FILINFO info;
    return f_stat(path, &info) == FR_OK ? (uint32_t)info.fsize : 0;
}

/** Write @p size bytes of 0xFF in @p chunk sized pieces. */
static bool write_blank(FIL *file, uint32_t size, uint32_t chunk)
{
    UINT written;
    memset(g_scratch, 0xFF, chunk);
    for (uint32_t done = 0; done < size; done += chunk) {
        if (f_write(file, g_scratch, chunk, &written) != FR_OK || written != chunk) {
            return false;
        }
    }
    return true;
}

bool save_file_create(const char *path, uint32_t size)
{
    FIL file;
    if (size == 0 || f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return false;
    }
    /* Small saves (EEPROM) are written in sectors, larger ones in 2 KiB steps,
     * as the original kernel did; all real save sizes are multiples of these. */
    bool ok = write_blank(&file, size, size < 0x800 ? 0x200 : 0x800);
    f_close(&file);
    return ok;
}

bool save_file_load_to_sram(const char *path)
{
    FIL file;
    UINT read;

    if (f_open(&file, path, FA_READ) != FR_OK) {
        return false;
    }
    uint32_t size = f_size(&file);
    if (size > SAVE_MAX_SIZE) {
        size = SAVE_MAX_SIZE;
    }

    omega_set_sram_page(SRAM_PAGE_SAVE_LOW);
    if (size > SRAM_WINDOW_SIZE) {
        f_read(&file, g_scratch, SRAM_WINDOW_SIZE, &read);
        sram_write(SRAM_BASE, g_scratch, SRAM_WINDOW_SIZE);
        omega_set_sram_page(SRAM_PAGE_SAVE_HIGH);
        f_read(&file, g_scratch, size - SRAM_WINDOW_SIZE, &read);
        sram_write(SRAM_BASE, g_scratch, size - SRAM_WINDOW_SIZE);
    } else {
        f_read(&file, g_scratch, size, &read);
        sram_write(SRAM_BASE, g_scratch, size);
    }
    f_close(&file);
    omega_set_sram_page(SRAM_PAGE_SAVE_LOW);
    return true;
}

/** Copy a save-state file into its SRAM pages. */
static void load_save_state_to_sram(const char *path)
{
    FIL file;
    UINT read;

    if (f_open(&file, path, FA_READ) != FR_OK) {
        return;
    }
    for (u16 page = SRAM_PAGE_RTS_FIRST; page < SRAM_PAGE_RTS_END; page += SRAM_PAGE_STEP) {
        omega_set_sram_page(page);
        f_read(&file, g_scratch, SRAM_WINDOW_SIZE, &read);
        sram_write(SRAM_BASE, g_scratch, SRAM_WINDOW_SIZE);
    }
    f_close(&file);
    omega_set_sram_page(SRAM_PAGE_SAVE_LOW);
}

bool save_state_prepare(const char *game_filename, bool *created)
{
    char path[PATH_MAX_LEN];
    FIL file;

    *created = false;
    if (!sd_ensure_folder(SD_DIR_SAVE_STATES) ||
        !sd_companion_path(path, sizeof(path), SD_DIR_SAVE_STATES, game_filename, "rts")) {
        return false;
    }

    if (save_file_size(path) == 0) {
        *created = true;
        if (f_open(&file, path, FA_WRITE | FA_OPEN_ALWAYS) != FR_OK) {
            return false;
        }
        bool ok = write_blank(&file, SAVE_STATE_FILE_SIZE, 0x800);
        f_close(&file);
        if (!ok) {
            return false;
        }
    }

    load_save_state_to_sram(path);
    return fat_map_add_file(&g_fs, path, FAT_MAP_SAVE_STATE) == FAT_MAP_OK;
}
