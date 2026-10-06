/**
 * @file game_screens.c
 * @brief Game page (start / copy / save type / cheats / delete) and the
 * cheat selection screen.
 */
#include <stdio.h>
#include <string.h>

#include "core/cheat.h"
#include "core/game_file.h"
#include "core/text.h"
#include "ff.h"
#include "loader/boot.h"
#include "loader/buffers.h"
#include "loader/directory.h"
#include "loader/game_info.h"
#include "loader/library_files.h"
#include "loader/save_files.h"
#include "loader/sd_paths.h"
#include "ui/app.h"
#include "ui/theme.h"
#include "ui/widgets.h"

/* ---------------------------------------------------------- game panel -- */

#define ART_X 4
#define ART_Y (CONTENT_TOP + 4)
#define INFO_Y (ART_Y + THUMBNAIL_HEIGHT + 4)

void ui_draw_game_panel(const char game_code[4], const char *const lines[], unsigned count)
{
    const uint16_t *pixels = game_code[0] > ' ' ? thumbnail_load(game_code) : NULL;

    gfx_fill(0, CONTENT_TOP, GAME_PANEL_WIDTH, CONTENT_HEIGHT, COLOR_BACKGROUND);
    gfx_frame(ART_X - 1, ART_Y - 1, THUMBNAIL_WIDTH + 2, THUMBNAIL_HEIGHT + 2, COLOR_RULE);
    if (pixels) {
        gfx_blit(ART_X, ART_Y, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, pixels, THUMBNAIL_WIDTH);
    } else {
        gfx_fill(ART_X, ART_Y, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, COLOR_PLACEHOLDER);
        gfx_text(&FONT_SMALL, ART_X + THUMBNAIL_WIDTH / 2, ART_Y + THUMBNAIL_HEIGHT / 2 - 6,
                 COLOR_TEXT_MUTED, "No box art", 0, ALIGN_CENTER);
    }
    int y = INFO_Y;
    for (unsigned i = 0; i < count && y < CONTENT_BOTTOM; i++, y += font_line_height(&FONT_SMALL)) {
        gfx_text(&FONT_SMALL, ART_X, y, COLOR_TEXT_MUTED, lines[i], THUMBNAIL_WIDTH, ALIGN_LEFT);
    }
    gfx_fill(GAME_PANEL_WIDTH - 1, CONTENT_TOP, 1, CONTENT_HEIGHT, COLOR_RULE);
}

static void strip_extension(char *out, size_t size, const char *name)
{
    text_copy(out, size, name);
    if (game_file_is_gba(out)) {
        out[strlen(out) - 4] = '\0';
    }
}

/* ----------------------------------------------------------- game page -- */

typedef enum {
    OPT_PLAY,
    OPT_PLAY_HOOKS,
    OPT_NOR,
    OPT_NOR_HOOKS,
    OPT_SAVE_TYPE,
    OPT_FAVORITE,
    OPT_CHEATS,
    OPT_RESTORE,
    OPT_DELETE,
} option_t;

typedef struct {
    app_t *app;
    const char *path;
    game_info_t info;
    char name[FF_LFN_BUF + 1]; /**< Full file name (saves are named after it). */
    save_choice_t save_choice;
    bool detected;             /**< detected_mode holds a cached scan result. */
    save_mode_t detected_mode; /**< Save type found by scanning the game. */
    bool favorite;             /**< In the favorites list. */
    bool has_cheats;
    char cheat_path[PATH_MAX_LEN];
    unsigned cheats_selected;
    option_t options[10];
    unsigned option_count;
} game_page_t;

static void game_row(void *ctx, unsigned index, ui_row_t *row)
{
    const game_page_t *page = ctx;
    bool hooks = settings_any_hook(&page->app->settings);
    switch (page->options[index]) {
    case OPT_PLAY:
        text_copy(row->label, sizeof(row->label), "Play");
        break;
    case OPT_PLAY_HOOKS:
        text_copy(row->label, sizeof(row->label), "Play + add-ons");
        row->dimmed = !hooks;
        break;
    case OPT_NOR:
        text_copy(row->label, sizeof(row->label), "Copy to NOR");
        break;
    case OPT_NOR_HOOKS:
        text_copy(row->label, sizeof(row->label), "NOR + add-ons");
        row->dimmed = !hooks;
        break;
    case OPT_SAVE_TYPE:
        text_copy(row->label, sizeof(row->label), "Save");
        text_copy(row->value, sizeof(row->value), save_choice_name(page->save_choice));
        break;
    case OPT_FAVORITE:
        text_copy(row->label, sizeof(row->label), "Favorite");
        text_copy(row->value, sizeof(row->value), page->favorite ? "Yes" : "No");
        break;
    case OPT_CHEATS:
        text_copy(row->label, sizeof(row->label), "Cheats");
        if (page->cheats_selected) {
            snprintf(row->value, sizeof(row->value), "%u on", page->cheats_selected);
        } else {
            text_copy(row->value, sizeof(row->value), "Off");
        }
        row->kind = ROW_CHEVRON;
        break;
    case OPT_RESTORE:
        text_copy(row->label, sizeof(row->label), "Restore save");
        break;
    case OPT_DELETE:
        text_copy(row->label, sizeof(row->label), "Delete");
        break;
    }
}

/** Button hints for the selected option. */
static void game_hints(const game_page_t *page, const ui_list_t *list)
{
    bool hooks = settings_any_hook(&page->app->settings);
    switch (page->options[ui_list_selected(list)]) {
    case OPT_PLAY:
        ui_hints("A Play|L+A BIOS|B Back");
        break;
    case OPT_PLAY_HOOKS:
        ui_hints(hooks ? "A Play|L+A BIOS|B Back" : "A Why off?|B Back");
        break;
    case OPT_NOR:
        ui_hints("A Copy|B Back");
        break;
    case OPT_NOR_HOOKS:
        ui_hints(hooks ? "A Copy|B Back" : "A Why off?|B Back");
        break;
    case OPT_SAVE_TYPE:
        ui_hints("<> Change|B Back");
        break;
    case OPT_FAVORITE:
        ui_hints("A Change|B Back");
        break;
    case OPT_CHEATS:
        ui_hints("A Choose|B Back");
        break;
    case OPT_RESTORE:
        ui_hints("A Restore|B Back");
        break;
    case OPT_DELETE:
        ui_hints("A Delete|B Back");
        break;
    }
}

static void draw_game_page(game_page_t *page, ui_list_t *list)
{
    char title[64];
    char line1[32];
    char size[16];
    char line2[32];
    save_mode_t mode = save_type_resolve(page->save_choice, page->info.game_code, page->info.size);
    const char *save = save_mode_name(mode);
    if (page->save_choice == SAVE_CHOICE_AUTO && !save_type_known(page->info.game_code)) {
        /* Not in the database: found when the game is first started. */
        save = page->detected ? save_mode_name(page->detected_mode) : "Found at start";
    }

    strip_extension(title, sizeof(title), page->name);
    text_format_size(size, sizeof(size), page->info.size);
    if (page->info.game_code[0] > ' ') {
        snprintf(line1, sizeof(line1), "%s   %s", page->info.game_code, size);
    } else {
        text_copy(line1, sizeof(line1), size); /* homebrew often has no game code */
    }
    snprintf(line2, sizeof(line2), "Save: %s", save);
    const char *const lines[] = {line1, line2};

    ui_title_bar(title, NULL);
    ui_draw_game_panel(page->info.game_code, lines, 2);
    ui_list_draw(list);
    game_hints(page, list);
}

static void start(game_page_t *page, boot_action_t action, bool bios_boot)
{
    boot_request_t request = {
        .path = page->path,
        .action = action,
        .save_choice = page->save_choice,
        .bios_boot = bios_boot,
    };
    bool to_nor = action == BOOT_COPY_TO_NOR || action == BOOT_COPY_TO_NOR_WITH_HOOKS;
    const progress_t *progress = ui_progress_begin(to_nor ? "Copying to NOR" : "Starting");
    boot_result_t result = boot_sd_game(&request, &page->app->settings, progress);
    if (result == BOOT_OK) {
        ui_message("Copied", "The game is now in NOR flash and starts without loading.");
    } else {
        ui_message(to_nor ? "Can't Copy Game" : "Can't Start Game", boot_result_message(result));
    }
    /* Starting may have detected the save type. */
    page->detected =
        save_detected_read(page->name, page->info.game_code, page->info.size, &page->detected_mode);
}

/** The favorites list lives in the scratch buffer while it is changed. */
#define FAVORITES ((favorites_t *)g_scratch)
_Static_assert(sizeof(favorites_t) <= SCRATCH_SIZE, "favorites must fit the scratch buffer");

static void toggle_favorite(game_page_t *page)
{
    favorites_file_load(FAVORITES);
    if (page->favorite) {
        favorites_remove(FAVORITES, page->path);
    } else if (!favorites_add(FAVORITES, page->path)) {
        ui_message("Favorites Full", "Up to 64 games can be favorites. Remove one first.");
        return;
    }
    if (favorites_file_save(FAVORITES)) {
        page->favorite = !page->favorite;
    } else {
        ui_message("Can't Save", "The favorites could not be written to the SD card.");
    }
}

static void explain_add_ons(void)
{
    ui_message("Add-ons Are Off",
               "Turn on return to menu, save states, sleep or cheats under Settings first.");
}

void ui_game_page(app_t *app, const char *path)
{
    static game_page_t page;
    ui_list_t list;
    input_t input;

    memset(&page, 0, sizeof(page));
    page.app = app;
    page.path = path;
    sd_long_name(path, page.name, sizeof(page.name));
    game_info_read(path, &page.info);
    page.save_choice = save_choice_read(page.name);
    page.detected =
        save_detected_read(page.name, page.info.game_code, page.info.size, &page.detected_mode);
    favorites_file_load(FAVORITES);
    page.favorite = favorites_find(FAVORITES, path) >= 0;
    g_cheat_code_count = 0;
    page.has_cheats =
        app->settings.cheats && cheat_file_find(path, page.cheat_path, sizeof(page.cheat_path));

    page.options[page.option_count++] = OPT_PLAY;
    page.options[page.option_count++] = OPT_PLAY_HOOKS;
    page.options[page.option_count++] = OPT_NOR;
    page.options[page.option_count++] = OPT_NOR_HOOKS;
    page.options[page.option_count++] = OPT_SAVE_TYPE;
    page.options[page.option_count++] = OPT_FAVORITE;
    if (page.has_cheats) {
        page.options[page.option_count++] = OPT_CHEATS;
    }
    if (save_backup_exists(page.name)) {
        page.options[page.option_count++] = OPT_RESTORE;
    }
    page.options[page.option_count++] = OPT_DELETE;

    platform_set_key_repeat(LIST_REPEAT_DELAY, LIST_REPEAT_RATE);
    ui_list_init(&list, page.option_count, 0, game_row, &page, CONTENT_TOP, ROW_HEIGHT);
    ui_list_set_column(&list, GAME_PANEL_WIDTH, GFX_WIDTH - GAME_PANEL_WIDTH);
    draw_game_page(&page, &list);

    for (;;) {
        platform_wait_vblank();
        ui_tick();
        platform_read_input(&input);

        option_t option = page.options[ui_list_selected(&list)];
        /* L is the BIOS-boot modifier here, and the list is short: no paging. */
        input_t nav = input;
        nav.repeated &= (uint16_t) ~(BTN_L | BTN_R);

        if (option == OPT_SAVE_TYPE && (input.repeated & (BTN_LEFT | BTN_RIGHT))) {
            int step = (input.repeated & BTN_LEFT) ? SAVE_CHOICE_COUNT - 1 : 1;
            page.save_choice = (save_choice_t)((page.save_choice + step) % SAVE_CHOICE_COUNT);
            draw_game_page(&page, &list);
            continue;
        }

        ui_list_event_t event = ui_list_update(&list, &nav);
        game_hints(&page, &list);
        if (event == UI_LIST_BACK) {
            return;
        }
        if (event != UI_LIST_ACTIVATE) {
            continue;
        }

        bool bios_boot = (input.held & BTN_L) != 0;
        bool hooks = settings_any_hook(&app->settings);
        switch (option) {
        case OPT_PLAY:
            start(&page, BOOT_PLAY, bios_boot);
            break;
        case OPT_PLAY_HOOKS:
            if (hooks) {
                start(&page, BOOT_PLAY_WITH_HOOKS, bios_boot);
            } else {
                explain_add_ons();
            }
            break;
        case OPT_NOR:
            start(&page, BOOT_COPY_TO_NOR, false);
            break;
        case OPT_NOR_HOOKS:
            if (hooks) {
                start(&page, BOOT_COPY_TO_NOR_WITH_HOOKS, false);
            } else {
                explain_add_ons();
            }
            break;
        case OPT_SAVE_TYPE:
            continue; /* changed with left / right */
        case OPT_FAVORITE:
            toggle_favorite(&page);
            break;
        case OPT_CHEATS:
            page.cheats_selected = ui_cheats(page.cheat_path);
            break;
        case OPT_RESTORE:
            if (ui_confirm("Restore Save",
                           "Go back to the save from before the last start? The current save "
                           "becomes the backup, so you can switch back.",
                           "Restore") &&
                !save_backup_restore(page.name)) {
                ui_message("Can't Restore Save", "The save files could not be renamed.");
            }
            break;
        case OPT_DELETE: {
            char question[160];
            char title[64];
            strip_extension(title, sizeof(title), page.name);
            snprintf(question, sizeof(question),
                     "Delete \"%s\" from the SD card? Its save file is kept.", title);
            if (ui_confirm("Delete Game", question, "Delete")) {
                f_unlink(path);
                return;
            }
            break;
        }
        }
        platform_set_key_repeat(LIST_REPEAT_DELAY, LIST_REPEAT_RATE);
        draw_game_page(&page, &list);
    }
}

/* -------------------------------------------------------------- cheats -- */

/** The cheat menu lives in the scratch buffer while this screen is open. */
#define CHEAT_ENTRIES ((cht_entry_t *)(g_scratch + 0x2000))
#define CHEAT_MAX_ENTRIES ((SCRATCH_SIZE - 0x2000) / sizeof(cht_entry_t))

typedef struct {
    size_t count;
} cheat_page_t;

static void cheat_row(void *ctx, unsigned index, ui_row_t *row)
{
    (void)ctx;
    const cht_entry_t *e = &CHEAT_ENTRIES[index];
    text_copy(row->label, sizeof(row->label), e->label);
    row->kind = e->is_section ? ROW_HEADING : ROW_CHECK;
    row->checked = e->selected;
}

unsigned ui_cheats(const char *cheat_path)
{
    cheat_file_t file;
    cheat_page_t page;
    ui_list_t list;
    input_t input;
    char title[CHT_NAME_LEN];

    if (!cheat_file_open(&file, cheat_path)) {
        ui_message("Cheats", "The cheat file can't be opened.");
        return 0;
    }
    cht_read_game_name(&file.reader, title, sizeof(title));
    page.count = cht_list_entries(&file.reader, CHEAT_ENTRIES, CHEAT_MAX_ENTRIES);

    platform_set_key_repeat(LIST_REPEAT_DELAY, LIST_REPEAT_RATE);
    ui_title_bar(title[0] ? title : "Cheats", NULL);
    ui_hints("A On / off|B Done");
    ui_list_init(&list, (unsigned)page.count, 0, cheat_row, &page, CONTENT_TOP, ROW_HEIGHT);
    list.empty_text = "This cheat file has no cheats.";
    ui_list_draw(&list);

    for (;;) {
        platform_wait_vblank();
        ui_tick();
        platform_read_input(&input);
        ui_list_event_t event = ui_list_update(&list, &input);
        if (event == UI_LIST_ACTIVATE) {
            cht_toggle_option(CHEAT_ENTRIES, page.count, ui_list_selected(&list));
            ui_list_draw(&list);
        } else if (event == UI_LIST_BACK) {
            break;
        }
    }

    g_cheat_code_count =
        (uint32_t)cht_collect_codes(&file.reader, CHEAT_ENTRIES, page.count, &g_cheat_workspace,
                                    g_cheat_codes, MAX_CHEAT_CODES);
    unsigned selected = (unsigned)cht_selected_count(CHEAT_ENTRIES, page.count);
    cheat_file_close(&file);
    return selected;
}
