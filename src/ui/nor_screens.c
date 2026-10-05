/**
 * @file nor_screens.c
 * @brief The NOR flash library: list, game page, delete and erase.
 */
#include <stdio.h>
#include <string.h>

#include "core/game_file.h"
#include "core/text.h"
#include "loader/boot.h"
#include "loader/library_files.h"
#include "loader/nor_games.h"
#include "ui/app.h"
#include "ui/theme.h"
#include "ui/widgets.h"

typedef struct {
    unsigned count;
    uint32_t used;
} nor_state_t;

static void nor_name(char *out, size_t size, unsigned index)
{
    text_copy(out, size, g_nor_table[index].filename);
    if (game_file_is_gba(out)) {
        out[strlen(out) - 4] = '\0';
    }
}

static void library_row(void *ctx, unsigned index, ui_row_t *row)
{
    const nor_state_t *st = ctx;
    if (index < st->count) {
        nor_name(row->label, sizeof(row->label), index);
        text_format_size(row->value, sizeof(row->value), g_nor_table[index].size);
        row->kind = ROW_CHEVRON;
    } else {
        text_copy(row->label, sizeof(row->label), "Erase all games");
        row->kind = ROW_PLAIN;
    }
}

/* ------------------------------------------------------------ erasing -- */

static unsigned s_erase_frames;

static void erase_poll(uint32_t tick)
{
    char text[40];
    (void)tick;
    platform_wait_vblank();
    platform_wait_vblank();
    s_erase_frames += 2;
    if (s_erase_frames % 60 < 2) {
        unsigned seconds = s_erase_frames / 60;
        snprintf(text, sizeof(text), "Erasing... %u:%02u", seconds / 60, seconds % 60);
        gfx_fill(0, 60, GFX_WIDTH, 20, COLOR_BACKGROUND);
        gfx_text(&FONT_BODY, GFX_WIDTH / 2, 62, COLOR_TEXT, text, 0, ALIGN_CENTER);
    }
}

static void erase_all(void)
{
    if (!ui_confirm("Erase NOR Flash",
                    "Erase every game in NOR flash? Save files on the SD card are kept. "
                    "This takes about 4 minutes.",
                    "Erase")) {
        return;
    }
    ui_progress_begin("Erasing");
    gfx_text(&FONT_SMALL, GFX_WIDTH / 2, 100, COLOR_TEXT_MUTED, "Don't turn the GBA off.", 0,
             ALIGN_CENTER);
    s_erase_frames = 0;
    nor_games_erase_all(erase_poll);
    ui_message("NOR Flash Erased", "All games were removed from NOR flash.");
}

/* ---------------------------------------------------------- game page -- */

typedef struct {
    bool deletable;
} nor_page_t;

static void nor_page_row(void *ctx, unsigned index, ui_row_t *row)
{
    const nor_page_t *page = ctx;
    if (index == 0) {
        text_copy(row->label, sizeof(row->label), "Play");
    } else {
        text_copy(row->label, sizeof(row->label), "Delete");
        row->dimmed = !page->deletable;
    }
}

static void draw_nor_page(unsigned index, ui_list_t *list)
{
    char title[64];
    char line1[32];
    char size[16];
    char code[5];

    nor_name(title, sizeof(title), index);
    memcpy(code, g_nor_table[index].header_title + 0xC, 4);
    code[4] = '\0';
    snprintf(line1, sizeof(line1), "%s   %s", code,
             text_format_size(size, sizeof(size), g_nor_table[index].size));
    const char *const lines[] = {line1,
                                 g_nor_table[index].has_hooks ? "With add-ons" : "No add-ons"};

    ui_title_bar(title, NULL);
    ui_draw_game_panel(code, lines, 2);
    ui_list_draw(list);
    ui_hints(ui_list_selected(list) == 0 ? "A Play|L+A BIOS|B Back" : "A Delete|B Back");
}

/** @return true if the library changed. */
static bool nor_game_page(app_t *app, nor_state_t *st, unsigned index)
{
    nor_page_t page = {index + 1 == st->count};
    ui_list_t list;
    input_t input;

    ui_list_init(&list, 2, 0, nor_page_row, &page, CONTENT_TOP, ROW_HEIGHT);
    ui_list_set_column(&list, GAME_PANEL_WIDTH, GFX_WIDTH - GAME_PANEL_WIDTH);
    draw_nor_page(index, &list);

    for (;;) {
        platform_wait_vblank();
        ui_tick();
        platform_read_input(&input);
        input_t nav = input;
        nav.repeated &= (uint16_t) ~(BTN_L | BTN_R);
        ui_list_event_t event = ui_list_update(&list, &nav);
        ui_hints(ui_list_selected(&list) == 0 ? "A Play|L+A BIOS|B Back" : "A Delete|B Back");
        switch (event) {
        case UI_LIST_BACK:
            return false;
        case UI_LIST_IDLE:
            continue;
        case UI_LIST_ACTIVATE:
            break;
        }

        if (ui_list_selected(&list) == 0) {
            const progress_t *progress = ui_progress_begin("Starting");
            boot_result_t r =
                boot_nor_game(index, (input.held & BTN_L) != 0, &app->settings, progress);
            ui_message("Can't Start Game", boot_result_message(r));
        } else if (!page.deletable) {
            ui_message("Delete from NOR",
                       "Only the game copied last can be deleted. Delete the games after it "
                       "first, or erase all games.");
        } else if (ui_confirm("Delete from NOR", "Delete this game from NOR flash?", "Delete")) {
            nor_game_delete_last(st->count, st->used);
            return true;
        }
        draw_nor_page(index, &list);
    }
}

/* ------------------------------------------------------------ library -- */

void ui_nor_library(app_t *app)
{
    nor_state_t st;
    ui_list_t list;
    input_t input;
    char free_text[20];
    unsigned selected = 0;

    for (;;) {
        platform_set_key_repeat(LIST_REPEAT_DELAY, LIST_REPEAT_RATE);
        nor_games_load_table();
        st.count = nor_games_scan(&st.used);
        snprintf(free_text, sizeof(free_text), "%lu MB free",
                 (unsigned long)((NOR_GAMES_CAPACITY - st.used) >> 20));
        ui_title_bar("NOR Flash", free_text);
        ui_hints(st.count ? "A Open|B Back" : "B Back");

        unsigned rows = st.count ? st.count + 1 : 0;
        ui_list_init(&list, rows, selected, library_row, &st, CONTENT_TOP, ROW_HEIGHT);
        list.empty_text = "No games in NOR flash. Choose \"Copy to NOR\" on a game to add it.";
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
        if (selected < st.count) {
            if (nor_game_page(app, &st, selected) && selected > 0) {
                selected--;
            }
        } else {
            erase_all();
            selected = 0;
        }
    }
}
