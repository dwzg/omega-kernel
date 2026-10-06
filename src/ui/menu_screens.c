/**
 * @file menu_screens.c
 * @brief Main menu, SD card browser and recently played list.
 */
#include <stdio.h>
#include <string.h>

#include "core/game_file.h"
#include "core/recent.h"
#include "core/text.h"
#include "loader/buffers.h"
#include "loader/directory.h"
#include "loader/library_files.h"
#include "loader/sd_paths.h"
#include "ui/app.h"
#include "ui/theme.h"
#include "ui/widgets.h"

/** Copy @p name without its ".gba" extension. */
static void display_name(char *out, size_t size, const char *name)
{
    text_copy(out, size, name);
    if (game_file_is_gba(out)) {
        out[strlen(out) - 4] = '\0';
    }
}

/* ----------------------------------------------------------- main menu -- */

typedef enum { MAIN_SD, MAIN_NOR, MAIN_RECENT, MAIN_SETTINGS, MAIN_ABOUT, MAIN_COUNT } main_item_t;

static void main_row(void *ctx, unsigned index, ui_row_t *row)
{
    static const char *const LABELS[MAIN_COUNT] = {
        "SD Card", "NOR Flash", "Recently Played", "Settings", "About",
    };
    (void)ctx;
    text_copy(row->label, sizeof(row->label), LABELS[index]);
    row->kind = ROW_CHEVRON;
}

void ui_main_menu(app_t *app)
{
    ui_list_t list;
    input_t input;
    unsigned selected = MAIN_SD;

    for (;;) {
        platform_set_key_repeat(LIST_REPEAT_DELAY, LIST_REPEAT_RATE);
        ui_title_bar("Omega Kernel", NULL);
        ui_hints("A Open");
        ui_list_init(&list, MAIN_COUNT, selected, main_row, NULL, CONTENT_TOP, ROW_HEIGHT);
        ui_list_draw(&list);

        ui_list_event_t event;
        do {
            platform_wait_vblank();
            ui_tick();
            platform_read_input(&input);
            event = ui_list_update(&list, &input);
        } while (event != UI_LIST_ACTIVATE);

        selected = ui_list_selected(&list);
        switch ((main_item_t)selected) {
        case MAIN_SD:
            ui_sd_browser(app);
            break;
        case MAIN_NOR:
            ui_nor_library(app);
            break;
        case MAIN_RECENT:
            ui_recent(app);
            break;
        case MAIN_SETTINGS:
            ui_settings(app);
            break;
        default:
            ui_about(app);
            break;
        }
    }
}

/* ---------------------------------------------------------- SD browser -- */

static void browser_row(void *ctx, unsigned index, ui_row_t *row)
{
    (void)ctx;
    const dir_entry_t *e = directory_entry(index);
    if (directory_is_folder(index)) {
        text_copy(row->label, sizeof(row->label), e->name);
        row->kind = ROW_CHEVRON;
    } else {
        display_name(row->label, sizeof(row->label), e->name);
        text_format_size(row->value, sizeof(row->value), e->size);
        row->kind = ROW_PLAIN;
    }
}

static const char *folder_title(const char *path)
{
    return strcmp(path, "/") == 0 ? "SD Card" : path_basename(path);
}

/** Button hints for the browser, depending on what is selected. */
static void browser_hints(const app_t *app, const ui_list_t *list)
{
    bool at_root = strcmp(app->sd_path, "/") == 0;
    bool folder = list->view.count > 0 && directory_is_folder(ui_list_selected(list));
    if (list->view.count == 0) {
        ui_hints(at_root ? "B Menu|START Recent" : "B Back|START Recent");
    } else if (folder) {
        ui_hints(at_root ? "A Open|B Menu|START Recent" : "A Open|B Back|START Recent");
    } else {
        ui_hints(at_root ? "A Select|B Menu|START Recent" : "A Select|B Back|START Recent");
    }
}

void ui_sd_browser(app_t *app)
{
    dir_listing_t listing;
    ui_list_t list;
    input_t input;
    char count_text[16];

    for (;;) {
        /* (Re)load the current folder. */
        platform_set_key_repeat(LIST_REPEAT_DELAY, LIST_REPEAT_RATE);
        directory_read(app->sd_path, &listing);
        unsigned total = listing.folders + listing.files;
        app_position_t *pos =
            &app->positions[app->depth < APP_MAX_DEPTH ? app->depth : APP_MAX_DEPTH - 1];

        ui_list_init(&list, total, 0, browser_row, NULL, CONTENT_TOP, ROW_HEIGHT);
        list.empty_text = "No games in this folder.";
        if (app->select_name[0]) {
            /* Select the last played game, or the folder we came back from. */
            int found = directory_find(app->select_name);
            if (found >= 0) {
                pos->selected = (uint16_t)found;
                pos->top = (uint16_t)(found >= LIST_ROWS / 2 ? found - LIST_ROWS / 2 : 0);
            }
            app->select_name[0] = '\0';
        }
        list_view_restore(&list.view, pos->selected, pos->top);

        bool reload = false;
        while (!reload) {
            snprintf(count_text, sizeof(count_text), "%u/%u",
                     total ? ui_list_selected(&list) + 1 : 0, total);
            ui_title_bar(folder_title(app->sd_path), count_text);
            browser_hints(app, &list);
            ui_list_draw(&list);

            bool redraw = false;
            while (!redraw && !reload) {
                platform_wait_vblank();
                ui_tick();
                platform_read_input(&input);
                unsigned before = ui_list_selected(&list);
                ui_list_event_t event = ui_list_update(&list, &input);
                pos->selected = (uint16_t)list.view.selected;
                pos->top = (uint16_t)list.view.top;
                if (ui_list_selected(&list) != before) {
                    snprintf(count_text, sizeof(count_text), "%u/%u", ui_list_selected(&list) + 1,
                             total);
                    ui_title_bar(folder_title(app->sd_path), count_text);
                    browser_hints(app, &list);
                }

                if (input.pressed & BTN_START) {
                    ui_recent(app);
                    reload = true; /* files may have changed (deleted) */
                } else if (event == UI_LIST_BACK) {
                    if (strcmp(app->sd_path, "/") == 0) {
                        return;
                    }
                    text_copy(app->select_name, sizeof(app->select_name),
                              path_basename(app->sd_path));
                    path_to_parent(app->sd_path);
                    if (app->depth > 0) {
                        app->depth--;
                    }
                    reload = true;
                } else if (event == UI_LIST_ACTIVATE) {
                    unsigned index = ui_list_selected(&list);
                    char path[PATH_MAX_LEN];
                    if (!path_join(path, sizeof(path), app->sd_path, directory_open_name(index))) {
                        ui_message("Can't Open", "The path is too long.");
                        redraw = true;
                    } else if (directory_is_folder(index)) {
                        text_copy(app->sd_path, sizeof(app->sd_path), path);
                        app->depth++;
                        if (app->depth < APP_MAX_DEPTH) {
                            app->positions[app->depth].selected = 0;
                            app->positions[app->depth].top = 0;
                        }
                        reload = true;
                    } else {
                        ui_game_page(app, path);
                        reload = true; /* the game may have been deleted */
                    }
                }
            }
        }
    }
}

/* ------------------------------------------------------ recently played -- */

/* Full names of the recently played games (a path may use a short name). */
static char s_recent_names[RECENT_MAX][DIR_NAME_LEN];

static void recent_row(void *ctx, unsigned index, ui_row_t *row)
{
    (void)ctx;
    display_name(row->label, sizeof(row->label), s_recent_names[index]);
    row->kind = ROW_CHEVRON;
}

void ui_recent(app_t *app)
{
    ui_list_t list;
    input_t input;
    unsigned selected = 0;

    for (;;) {
        platform_set_key_repeat(LIST_REPEAT_DELAY, LIST_REPEAT_RATE);
        recent_file_load(&g_recent);
        for (unsigned i = 0; i < g_recent.count; i++) {
            sd_long_name(g_recent.entries[i], s_recent_names[i], sizeof(s_recent_names[i]));
        }
        ui_title_bar("Recently Played", NULL);
        ui_hints("A Select|B Back");
        ui_list_init(&list, g_recent.count, selected, recent_row, NULL, CONTENT_TOP, ROW_HEIGHT);
        list.empty_text = "No games played yet.";
        ui_list_draw(&list);

        ui_list_event_t event;
        do {
            platform_wait_vblank();
            ui_tick();
            platform_read_input(&input);
            event = ui_list_update(&list, &input);
        } while (event == UI_LIST_IDLE);

        if (event == UI_LIST_BACK) {
            return;
        }
        selected = ui_list_selected(&list);
        char path[RECENT_ENTRY_LEN];
        text_copy(path, sizeof(path), g_recent.entries[selected]);
        ui_game_page(app, path);
    }
}
