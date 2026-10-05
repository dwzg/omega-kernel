/**
 * @file app.c
 * @brief Settings persistence and the interface's entry point.
 */
#include "ui/app.h"

#include "core/text.h"
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

void ui_run(app_t *app)
{
    text_copy(app->sd_path, sizeof(app->sd_path), "/");
    app->depth = 0;
    app->positions[0].selected = 0;
    app->positions[0].top = 0;

    /* Most of the time a game on the SD card is wanted: open it directly.
     * B at the root leads to the main menu. */
    ui_sd_browser(app);
    ui_main_menu(app);
}
