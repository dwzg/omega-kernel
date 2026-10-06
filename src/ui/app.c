/**
 * @file app.c
 * @brief Settings persistence and the interface's entry point.
 */
#include "ui/app.h"

#include <string.h>

#include "core/text.h"
#include "ff.h"
#include "loader/buffers.h"
#include "loader/library_files.h"
#include "platform/platform.h"

void app_load_settings(app_t *app)
{
    platform_settings_load(app->settings_words);
    settings_decode(&app->settings, app->settings_words);
}

void app_save_settings(app_t *app)
{
    settings_encode(&app->settings, app->settings_words);
    platform_settings_store(app->settings_words);
}

/**
 * Start where the user left off: open the folder of the last played game
 * with that game selected. Falls back to the top folder.
 */
static void resume_last_game(app_t *app)
{
    char folder[PATH_MAX_LEN];
    char name[sizeof(app->select_name)];
    FILINFO info;

    recent_file_load(&g_recent);
    if (g_recent.count == 0 ||
        !path_split(g_recent.entries[0], folder, sizeof(folder), name, sizeof(name))) {
        return;
    }
    if (strcmp(folder, "/") != 0 && (f_stat(folder, &info) != FR_OK || !(info.fattrib & AM_DIR))) {
        return; /* the folder is gone */
    }
    text_copy(app->sd_path, sizeof(app->sd_path), folder);
    text_copy(app->select_name, sizeof(app->select_name), name);
    app->depth = 0;
    for (const char *p = folder; *p; p++) {
        app->depth += (*p == '/' && p[1] != '\0');
    }
}

void ui_run(app_t *app)
{
    text_copy(app->sd_path, sizeof(app->sd_path), "/");
    app->depth = 0;
    app->select_name[0] = '\0';
    memset(app->positions, 0, sizeof(app->positions));
    resume_last_game(app);

    /* Most of the time a game on the SD card is wanted: open it directly.
     * B at the top folder leads to the main menu. */
    ui_sd_browser(app);
    ui_main_menu(app);
}
