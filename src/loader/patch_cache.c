/**
 * @file patch_cache.c
 * @brief `.pat` cache files. See patch_cache.h.
 */
#include "loader/patch_cache.h"

#include "core/path.h"
#include "ff.h"
#include "loader/save_files.h"
#include "loader/sd_paths.h"

bool patch_cache_load(patch_context_t *ctx, const char *game_filename)
{
    char path[PATH_MAX_LEN];
    uint32_t words[PATCH_PAT_FILE_SIZE / 4];
    FIL file;
    UINT read = 0;

    if (!sd_companion_path(path, sizeof(path), SD_DIR_PATCH_CACHE, game_filename, "pat") ||
        f_open(&file, path, FA_READ) != FR_OK) {
        return false;
    }
    f_read(&file, words, sizeof(words), &read);
    f_close(&file);
    return patch_state_deserialize(ctx, words, read / 4);
}

bool patch_cache_store(const patch_context_t *ctx, const char *game_filename)
{
    char path[PATH_MAX_LEN];
    uint32_t words[PATCH_PAT_FILE_SIZE / 4];
    FIL file;
    UINT written = 0;

    if (!sd_ensure_folder(SD_DIR_PATCH_CACHE) ||
        !sd_companion_path(path, sizeof(path), SD_DIR_PATCH_CACHE, game_filename, "pat") ||
        f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return false;
    }
    patch_state_serialize(ctx, words);
    f_write(&file, words, sizeof(words), &written);
    f_close(&file);
    return written == sizeof(words);
}
