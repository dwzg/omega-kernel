/**
 * @file app.h
 * @brief State shared by the screens, and the screens' entry points.
 *
 * Screen map (B always goes back):
 *
 *     Main menu
 *     ├── SD Card ──── folders ──── Game page ──── Cheats
 *     ├── NOR Flash ── Game page (NOR)
 *     ├── Recently Played ──────── Game page
 *     ├── Settings ─── Date & Time, Sleep Key, Menu Key
 *     └── About
 *
 * At start-up the kernel opens the SD card at the last played game (or the
 * top folder); START in the SD browser opens Recently Played.
 */
#ifndef UI_APP_H
#define UI_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "core/path.h"
#include "core/settings.h"

/** Folder depth whose list positions are remembered. */
#define APP_MAX_DEPTH 32

/** Scroll position of one folder level. */
typedef struct {
    uint16_t selected;
    uint16_t top;
} app_position_t;

typedef struct {
    settings_t settings;
    uint16_t settings_words[SETTINGS_WORDS]; /**< Raw block, keeps unknown words. */
    uint16_t fpga_version;
    char sd_path[PATH_MAX_LEN];              /**< Folder shown by the SD browser. */
    app_position_t positions[APP_MAX_DEPTH]; /**< Per depth, root = 0. */
    unsigned depth;
    /** Entry to select when the browser next opens @ref sd_path ("" = none). */
    char select_name[PATH_MAX_LEN];
} app_t;

/** @brief Load settings from flash into @p app. */
void app_load_settings(app_t *app);

/** @brief Save @p app's settings to flash. */
void app_save_settings(app_t *app);

/** @brief Run the interface (never returns). */
void ui_run(app_t *app) __attribute__((noreturn));

/** @name Screens */
/**@{*/
/** Main menu (never returns). */
void ui_main_menu(app_t *app) __attribute__((noreturn));
void ui_sd_browser(app_t *app);
void ui_recent(app_t *app);
void ui_favorites(app_t *app);
void ui_nor_library(app_t *app);
void ui_settings(app_t *app);
void ui_about(app_t *app);
/** Game page for a game on the SD card. */
void ui_game_page(app_t *app, const char *path);
/** Cheat selection; returns the number of selected options. */
unsigned ui_cheats(const char *cheat_path);
/** Edit the clock. */
void ui_edit_datetime(void);
/** Edit a hotkey; returns true if it was changed. */
bool ui_edit_hotkey(const char *title, const char *hint, uint8_t keys[HOTKEY_BUTTONS]);
/** Width of the left column of a game page (box art and details). */
#define GAME_PANEL_WIDTH 128
/**
 * Draw the left column of a game page: box art at full size (or a
 * placeholder) and @p count short lines of details below it.
 */
void ui_draw_game_panel(const char game_code[4], const char *const lines[], unsigned count);
/** Plain screen shown while the SD card is mounted. */
void ui_startup_screen(void);
/** Offer a firmware update at start-up. */
void ui_firmware_update(uint16_t current_version);
/** Show a fatal error and stop. */
void ui_fatal(const char *title, const char *text) __attribute__((noreturn));
/**@}*/

#endif /* UI_APP_H */
