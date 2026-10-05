/**
 * @file boot.h
 * @brief Starting a game: everything between "Play" and the jump into the game.
 *
 * Steps for a game on the SD card (see docs/boot-process.md):
 *  1. Map the ROM file for the FPGA (fat_map.h).
 *  2. Find or create the save file in /SAVER, map it and copy it into SRAM.
 *  3. Remember the game in the recently played list.
 *  4. Get the ROM into PSRAM: let the FPGA copy it, or copy it by software
 *     while scanning it for the patch engine.
 *  5. Install hooks / game fixes (patch.h), possibly from a `.pat` cache.
 *  6. Map PSRAM as the cartridge ROM and reset into the game.
 *
 * "Copy to NOR" runs steps 1-2 of a later boot ahead of time: the ROM is
 * written (and patched) into NOR flash once, and starts without loading.
 */
#ifndef LOADER_BOOT_H
#define LOADER_BOOT_H

#include <stdbool.h>

#include "core/save_type.h"
#include "core/settings.h"
#include "loader/progress.h"

/** What to do with a game from the SD card. */
typedef enum {
    BOOT_PLAY,                  /**< Start without hooks ("clean boot"). */
    BOOT_PLAY_WITH_HOOKS,       /**< Start with the hooks enabled in settings. */
    BOOT_COPY_TO_NOR,           /**< Write to NOR without hooks. */
    BOOT_COPY_TO_NOR_WITH_HOOKS /**< Write to NOR with hooks. */
} boot_action_t;

/** Outcome of a boot attempt. On success a game boot never returns. */
typedef enum {
    BOOT_OK,              /**< Only returned after a successful NOR copy. */
    BOOT_ERR_GAME_FILE,   /**< The game file cannot be read. */
    BOOT_ERR_TOO_LARGE,   /**< The game is larger than 32 MiB. */
    BOOT_ERR_FRAGMENTED,  /**< The ROM or save file has too many fragments. */
    BOOT_ERR_SAVE_FOLDER, /**< /SAVER cannot be created. */
    BOOT_ERR_SAVE_CREATE, /**< The save file cannot be created. */
    BOOT_ERR_SAVE_READ,   /**< The save file cannot be read. */
    BOOT_ERR_SAVE_EMPTY,  /**< The save file has no data. */
    BOOT_ERR_SAVE_STATE,  /**< The save-state file cannot be prepared. */
    BOOT_ERR_NOR_MISSING, /**< NOR flash not detected. */
    BOOT_ERR_NOR_FULL,    /**< Not enough room in NOR. */
} boot_result_t;

/** A request to start (or copy) a game from the SD card. */
typedef struct {
    const char *path; /**< Absolute path of the .gba file. */
    boot_action_t action;
    save_choice_t save_choice; /**< Save type chosen in the game menu. */
    bool bios_boot;            /**< Restart through the BIOS (L held). */
} boot_request_t;

/**
 * @brief Start or copy a game from the SD card.
 * @param settings Hooks, hotkeys and options; the cheat codes come from
 *                 ::g_cheat_codes.
 * @return Only returns for NOR copies and on errors.
 */
boot_result_t boot_sd_game(const boot_request_t *request, const settings_t *settings,
                           const progress_t *progress);

/**
 * @brief Start game @p index of the NOR library.
 * @return Only returns on errors.
 */
boot_result_t boot_nor_game(unsigned index, bool bios_boot, const settings_t *settings,
                            const progress_t *progress);

/** @brief A short, user-facing explanation of @p result. */
const char *boot_result_message(boot_result_t result);

#endif /* LOADER_BOOT_H */
