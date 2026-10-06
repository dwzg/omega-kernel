/**
 * @file boot.c
 * @brief Game start pipeline. See boot.h and docs/boot-process.md.
 */
#include "loader/boot.h"

#include <string.h>

#include "core/path.h"
#include "core/recent.h"
#include "ff.h"
#include "hal/omega.h"
#include "hal/platform.h"
#include "hal/sram.h"
#include "loader/buffers.h"
#include "loader/fat_map.h"
#include "loader/game_info.h"
#include "loader/library_files.h"
#include "loader/nor_games.h"
#include "loader/patch_cache.h"
#include "loader/patch_platform.h"
#include "loader/rom_loader.h"
#include "loader/save_files.h"
#include "loader/sd_paths.h"
#include "patch/payloads.h"
#include "platform/attributes.h"

/** Patch engine state for the game being started (too big for the stack). */
static patch_context_t s_patch PLATFORM_EWRAM;

static boot_result_t map_result(fat_map_result_t r, boot_result_t cannot_open)
{
    switch (r) {
    case FAT_MAP_OK:
        return BOOT_OK;
    case FAT_MAP_TOO_FRAGMENTED:
        return BOOT_ERR_FRAGMENTED;
    default:
        return cannot_open;
    }
}

/**
 * Find or create the save file and, when the game is about to start, map it
 * and copy it into SRAM.
 * @param save_size Receives the save size (0 = the game has no save memory).
 */
static boot_result_t prepare_save(const char *game_name, save_mode_t mode, bool starting,
                                  uint32_t *save_size, const progress_t *progress)
{
    char path[PATH_MAX_LEN];

    progress_status(progress, "Checking save file");
    if (!sd_ensure_folder(SD_DIR_SAVES) || !save_file_path(path, sizeof(path), game_name)) {
        return BOOT_ERR_SAVE_FOLDER;
    }

    *save_size = save_file_size(path);
    if (*save_size > 0 && starting) {
        progress_status(progress, "Backing up save");
        save_file_backup(game_name);
    }
    if (*save_size == 0) {
        *save_size = save_type_file_size(mode);
        if (*save_size == 0) {
            return BOOT_OK; /* no save memory, no file */
        }
        progress_status(progress, "Creating save file");
        if (!save_file_create(path, *save_size)) {
            return BOOT_ERR_SAVE_CREATE;
        }
    }

    if (!starting) {
        return BOOT_OK;
    }
    boot_result_t r = map_result(fat_map_add_file(&g_fs, path, FAT_MAP_SAVE), BOOT_ERR_SAVE_READ);
    if (r != BOOT_OK) {
        return r;
    }
    if (!fat_map_save_present()) {
        return BOOT_ERR_SAVE_EMPTY;
    }
    sram_flash_bank_switch(0);
    save_file_load_to_sram(path);
    return BOOT_OK;
}

static void remember_played(const char *path)
{
    recent_file_load(&g_recent);
    recent_touch(&g_recent, path);
    recent_file_save(&g_recent);
}

/** Size of the part of a PSRAM ROM the patch engine may read directly. */
static uint32_t psram_visible(uint32_t rom_size)
{
    return rom_size < ROM_WINDOW_SIZE ? rom_size : ROM_WINDOW_SIZE;
}

/* Never returns. */
static void start_psram_game(const settings_t *settings, bool bios_boot)
{
    omega_boot(PSRAM_ROM_PAGE, settings->game_rtc, bios_boot);
}

static boot_result_t play_clean(const boot_request_t *req, const game_info_t *info,
                                const settings_t *settings, const progress_t *progress)
{
    settings_t no_hooks = *settings;
    no_hooks.reset_hook = no_hooks.save_state_hook = no_hooks.sleep_hook = no_hooks.cheats = false;

    progress_status(progress, "Loading game");
    omega_send_fat_map(g_fat_map, OMEGA_FAT_MAP_COPY_ROM);
    patch_init(&s_patch, patch_platform_gba(), payloads_builtin(), info->game_code, &no_hooks, NULL,
               0);
    patch_apply_clean_psram(&s_patch, (uint32_t *)PSRAM_BASE, psram_visible(info->size));
    omega_set_auto_save(s_patch.st.auto_save);
    start_psram_game(settings, req->bios_boot);
    return BOOT_OK;
}

static boot_result_t play_with_hooks(const boot_request_t *req, const char *name,
                                     const game_info_t *info, save_mode_t save_mode,
                                     const settings_t *settings, const progress_t *progress)
{
    bool any_hook = settings_any_hook(settings);
    bool store_cache = false;

    if (settings->save_state_hook) {
        bool created;
        progress_status(progress, "Checking save state file");
        if (!save_state_prepare(name, &created)) {
            return BOOT_ERR_SAVE_STATE;
        }
    }

    progress_status(progress, "Checking patch cache");
    patch_init(&s_patch, patch_platform_gba(), payloads_builtin(), info->game_code, settings,
               g_cheat_codes, g_cheat_code_count);
    bool cached = patch_cache_load(&s_patch, name);

    progress_status(progress, "Loading game");
    if (cached) {
        omega_send_fat_map(g_fat_map, OMEGA_FAT_MAP_COPY_ROM);
    } else {
        rom_read_last_block(req->path, info->size);
        patch_find_trim_size(&s_patch, g_scratch, info->size, false, save_mode);
        if (settings->fast_patch && patch_use_irq_database(&s_patch)) {
            omega_send_fat_map(g_fat_map, OMEGA_FAT_MAP_COPY_ROM);
        } else {
            /* Copy by software and scan for IRQ references on the way. */
            fat_map_set_copy_mode(FAT_MAP_ROM_ALREADY_LOADED);
            omega_send_fat_map(g_fat_map, OMEGA_FAT_MAP_UPLOAD);
            rom_load_to_psram(req->path, any_hook ? &s_patch : NULL, progress);
            store_cache = true;
        }
    }

    if (any_hook) {
        patch_add_game_irq_fixes(&s_patch);
        patch_apply_hooks_psram(&s_patch, (uint32_t *)PSRAM_BASE, psram_visible(info->size));
        omega_set_auto_save(s_patch.st.auto_save);
    }
    if (store_cache) {
        progress_status(progress, "Saving patch cache");
        patch_cache_store(&s_patch, name);
    }
    start_psram_game(settings, req->bios_boot);
    return BOOT_OK;
}

static boot_result_t copy_to_nor(const boot_request_t *req, const game_info_t *info,
                                 save_mode_t save_mode, const settings_t *settings,
                                 const progress_t *progress)
{
    bool with_hooks = req->action == BOOT_COPY_TO_NOR_WITH_HOOKS && settings_any_hook(settings);
    uint32_t used;
    unsigned count;

    nor_games_load_table();
    count = nor_games_scan(&used);

    patch_init(&s_patch, patch_platform_gba(), payloads_builtin(), info->game_code, settings,
               g_cheat_codes, g_cheat_code_count);
    if (with_hooks) {
        patch_add_game_irq_fixes(&s_patch);
        rom_read_last_block(req->path, info->size);
        patch_find_trim_size(&s_patch, g_scratch, info->size, true, save_mode);
    }

    progress_status(progress, "Writing to NOR");
    switch (nor_game_write(req->path, count, used, &s_patch, with_hooks, progress)) {
    case NOR_WRITE_OK:
        return BOOT_OK;
    case NOR_WRITE_NO_CHIP:
        return BOOT_ERR_NOR_MISSING;
    case NOR_WRITE_OPEN_FAILED:
        return BOOT_ERR_GAME_FILE;
    default:
        return BOOT_ERR_NOR_FULL;
    }
}

boot_result_t boot_sd_game(const boot_request_t *req, const settings_t *settings,
                           const progress_t *progress)
{
    /* Saves and other companion files are named after the full name, also
     * when the game was opened by its 8.3 short name. */
    char name[FF_LFN_BUF + 1];
    sd_long_name(req->path, name, sizeof(name));
    bool starting = req->action == BOOT_PLAY || req->action == BOOT_PLAY_WITH_HOOKS;
    game_info_t info;
    boot_result_t r;

    fat_map_reset();
    if (!game_info_read(req->path, &info)) {
        return BOOT_ERR_GAME_FILE;
    }
    if (info.size > ROM_MAX_SIZE) {
        return BOOT_ERR_TOO_LARGE;
    }
    if (starting) {
        r = map_result(fat_map_add_file(&g_fs, req->path, FAT_MAP_ROM), BOOT_ERR_GAME_FILE);
        if (r != BOOT_OK) {
            return r;
        }
    }

    if (save_choice_read(name) != req->save_choice) {
        save_choice_write(name, req->save_choice);
    }
    save_mode_t save_mode =
        save_mode_for_game(req->save_choice, name, info.game_code, info.size, req->path, progress);

    uint32_t save_size;
    r = prepare_save(name, save_mode, starting, &save_size, progress);
    if (r != BOOT_OK) {
        return r;
    }
    if (!starting) {
        return copy_to_nor(req, &info, save_mode, settings, progress);
    }

    remember_played(req->path);
    fat_map_set_parameters(info.size, FAT_MAP_FPGA_COPIES_ROM, g_fs.csize, save_mode, save_size);
    if (req->action == BOOT_PLAY) {
        return play_clean(req, &info, settings, progress);
    }
    return play_with_hooks(req, name, &info, save_mode, settings, progress);
}

boot_result_t boot_nor_game(unsigned index, bool bios_boot, const settings_t *settings,
                            const progress_t *progress)
{
    const nor_entry_t *entry = &g_nor_table[index];
    char game_code[4];
    uint32_t save_size;

    fat_map_reset();
    memcpy(game_code, entry->header_title + 0xC, 4);
    save_mode_t save_mode = save_mode_for_game(save_choice_read(entry->filename), entry->filename,
                                               game_code, entry->size, NULL, NULL);

    boot_result_t r = prepare_save(entry->filename, save_mode, true, &save_size, progress);
    if (r != BOOT_OK) {
        return r;
    }
    fat_map_set_parameters(entry->size, FAT_MAP_ROM_ALREADY_LOADED, g_fs.csize, save_mode,
                           save_size);

    if (entry->has_hooks && entry->has_save_state) {
        bool created;
        progress_status(progress, "Checking save state file");
        if (!save_state_prepare(entry->filename, &created)) {
            return BOOT_ERR_SAVE_STATE;
        }
    }
    omega_send_fat_map(g_fat_map, OMEGA_FAT_MAP_UPLOAD);
    omega_boot(entry->rom_page, settings->game_rtc, bios_boot);
    return BOOT_OK;
}
