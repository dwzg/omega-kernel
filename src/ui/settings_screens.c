/**
 * @file settings_screens.c
 * @brief Settings list, the date & time and hotkey editors, and About.
 */
#include <stdio.h>
#include <string.h>

#include "core/datetime.h"
#include "core/settings.h"
#include "core/text.h"
#include "ui/app.h"
#include "ui/theme.h"
#include "ui/widgets.h"

/* ------------------------------------------------------------ editors -- */

/** Padding left and right of a field in the editors. */
#define FIELD_GAP 6
#define EDITOR_Y 78

/**
 * Draw a row of fields centred on the screen, the @p active one on the
 * selection colour with small arrows above and below.
 */
static void draw_fields(const char *const *fields, unsigned count, unsigned active,
                        const char *separator)
{
    int widths[8];
    int sep_width = separator ? gfx_text_width(&FONT_TITLE, separator) : 0;
    int total = 0;

    for (unsigned i = 0; i < count; i++) {
        widths[i] = gfx_text_width(&FONT_TITLE, fields[i]) + 2 * FIELD_GAP;
        total += widths[i] + (i + 1 < count ? sep_width : 0);
    }

    gfx_fill(0, EDITOR_Y - 12, GFX_WIDTH, 46, COLOR_BACKGROUND);
    int x = (GFX_WIDTH - total) / 2;
    for (unsigned i = 0; i < count; i++) {
        uint16_t color = COLOR_TEXT;
        if (i == active) {
            gfx_fill(x, EDITOR_Y - 2, widths[i], 20, COLOR_SELECTION);
            gfx_arrow(x + widths[i] / 2, EDITOR_Y - 9, COLOR_TEXT_MUTED, true);
            gfx_arrow(x + widths[i] / 2, EDITOR_Y + 23, COLOR_TEXT_MUTED, false);
            color = COLOR_TEXT_ON_SELECTION;
        }
        gfx_text(&FONT_TITLE, x + widths[i] / 2, EDITOR_Y + 1, color, fields[i], 0, ALIGN_CENTER);
        x += widths[i];
        if (separator && i + 1 < count) {
            gfx_text(&FONT_TITLE, x, EDITOR_Y + 1, COLOR_TEXT_MUTED, separator, 0, ALIGN_LEFT);
            x += sep_width;
        }
    }
}

static void draw_editor_frame(const char *title, const char *hint, const char *buttons)
{
    ui_title_bar(title, NULL);
    ui_clear_content();
    if (hint) {
        ui_draw_paragraph(16, CONTENT_TOP + 8, GFX_WIDTH - 32, hint, COLOR_TEXT_MUTED, true);
    }
    ui_hints(buttons);
}

/** Read the d-pad for an editor. @return 0, or +1/-1 for up/down. */
static int editor_step(const input_t *input, unsigned *field, unsigned count)
{
    if (input->repeated & BTN_LEFT) {
        *field = (*field + count - 1) % count;
    } else if (input->repeated & BTN_RIGHT) {
        *field = (*field + 1) % count;
    } else if (input->repeated & BTN_UP) {
        return 1;
    } else if (input->repeated & BTN_DOWN) {
        return -1;
    }
    return 0;
}

/** Add @p step to @p value, wrapping within [min, max]. */
static uint8_t wrap(int value, int step, int min, int max)
{
    value += step;
    if (value > max) {
        value = min;
    } else if (value < min) {
        value = max;
    }
    return (uint8_t)value;
}

enum { DT_YEAR, DT_MONTH, DT_DAY, DT_HOUR, DT_MINUTE, DT_FIELDS };

static void draw_datetime(datetime_t *dt, unsigned field)
{
    char text[DT_FIELDS][8];
    const char *fields[DT_FIELDS];

    datetime_sanitize(dt);
    snprintf(text[DT_YEAR], sizeof(text[0]), "20%02u", dt->year);
    snprintf(text[DT_MONTH], sizeof(text[0]), "%02u", dt->month);
    snprintf(text[DT_DAY], sizeof(text[0]), "%02u", dt->day);
    snprintf(text[DT_HOUR], sizeof(text[0]), "%02u", dt->hour);
    snprintf(text[DT_MINUTE], sizeof(text[0]), "%02u", dt->minute);
    for (unsigned i = 0; i < DT_FIELDS; i++) {
        fields[i] = text[i];
    }
    draw_fields(fields, DT_FIELDS, field, " ");
    gfx_fill(0, EDITOR_Y + 36, GFX_WIDTH, 16, COLOR_BACKGROUND);
    gfx_text(&FONT_SMALL, GFX_WIDTH / 2, EDITOR_Y + 36, COLOR_TEXT_MUTED,
             datetime_weekday_name(datetime_weekday(dt)), 0, ALIGN_CENTER);
}

void ui_edit_datetime(void)
{
    datetime_t dt;
    input_t input;
    unsigned field = DT_YEAR;

    platform_clock_read(&dt);
    dt.second = 0;
    platform_set_key_repeat(20, 5);
    draw_editor_frame("Date & Time", "Year, month, day, hour and minute.",
                      "<> Move|^v Change|A Save|B Cancel");
    draw_datetime(&dt, field);

    for (;;) {
        platform_wait_vblank();
        platform_read_input(&input);
        if (input.pressed & BTN_B) {
            return;
        }
        if (input.pressed & BTN_A) {
            dt.weekday = datetime_weekday(&dt);
            platform_clock_write(&dt);
            return;
        }

        unsigned before = field;
        int step = editor_step(&input, &field, DT_FIELDS);
        switch (step == 0 ? DT_FIELDS : field) {
        case DT_YEAR:
            dt.year = wrap(dt.year, step, 0, 99);
            break;
        case DT_MONTH:
            dt.month = wrap(dt.month, step, 1, 12);
            break;
        case DT_DAY:
            dt.day = wrap(dt.day, step, 1, days_in_month(dt.month, dt.year));
            break;
        case DT_HOUR:
            dt.hour = wrap(dt.hour, step, 0, 23);
            break;
        case DT_MINUTE:
            dt.minute = wrap(dt.minute, step, 0, 59);
            break;
        default:
            if (field == before) {
                continue;
            }
            break;
        }
        draw_datetime(&dt, field);
    }
}

bool ui_edit_hotkey(const char *title, const char *hint, uint8_t keys[HOTKEY_BUTTONS])
{
    uint8_t edited[HOTKEY_BUTTONS];
    const char *fields[HOTKEY_BUTTONS];
    input_t input;
    unsigned slot = 0;

    memcpy(edited, keys, sizeof(edited));
    platform_set_key_repeat(20, 6);
    draw_editor_frame(title, hint, "<> Move|^v Change|A Save|B Cancel");

    for (;;) {
        for (unsigned i = 0; i < HOTKEY_BUTTONS; i++) {
            fields[i] = settings_button_name(edited[i]);
        }
        draw_fields(fields, HOTKEY_BUTTONS, slot, "+");

        int step;
        unsigned before = slot;
        do {
            platform_wait_vblank();
            platform_read_input(&input);
            if (input.pressed & BTN_B) {
                return false;
            }
            if (input.pressed & BTN_A) {
                bool changed = memcmp(edited, keys, sizeof(edited)) != 0;
                memcpy(keys, edited, sizeof(edited));
                return changed;
            }
            step = editor_step(&input, &slot, HOTKEY_BUTTONS);
        } while (step == 0 && slot == before);

        if (step != 0) {
            settings_hotkey_step(edited, slot, step);
        }
    }
}

/* ----------------------------------------------------------- settings -- */

typedef enum {
    SET_HEADING_ADDONS,
    SET_RESET_HOOK,
    SET_SAVE_STATE_HOOK,
    SET_SLEEP_HOOK,
    SET_CHEATS,
    SET_HEADING_KEYS,
    SET_SLEEP_KEYS,
    SET_MENU_KEYS,
    SET_HEADING_SYSTEM,
    SET_DATETIME,
    SET_GAME_RTC,
    SET_FAST_PATCH,
    SET_LIST_ART,
    SET_COUNT
} setting_item_t;

static void settings_row(void *ctx, unsigned index, ui_row_t *row)
{
    const app_t *app = ctx;
    const settings_t *s = &app->settings;
    bool state_only = settings_save_state_only(s);
    datetime_t now;

    switch ((setting_item_t)index) {
    case SET_HEADING_ADDONS:
        text_copy(row->label, sizeof(row->label), "In-game add-ons");
        row->kind = ROW_HEADING;
        break;
    case SET_RESET_HOOK:
        text_copy(row->label, sizeof(row->label), "Return to menu");
        text_copy(row->value, sizeof(row->value), s->reset_hook ? "On" : "Off");
        break;
    case SET_SAVE_STATE_HOOK:
        text_copy(row->label, sizeof(row->label), "Save states");
        text_copy(row->value, sizeof(row->value), s->save_state_hook ? "On" : "Off");
        break;
    case SET_SLEEP_HOOK:
        text_copy(row->label, sizeof(row->label), "Sleep");
        text_copy(row->value, sizeof(row->value), s->sleep_hook ? "On" : "Off");
        break;
    case SET_CHEATS:
        text_copy(row->label, sizeof(row->label), "Cheats");
        text_copy(row->value, sizeof(row->value), s->cheats ? "On" : "Off");
        break;
    case SET_HEADING_KEYS:
        text_copy(row->label, sizeof(row->label), "Hotkeys");
        row->kind = ROW_HEADING;
        break;
    case SET_SLEEP_KEYS:
        text_copy(row->label, sizeof(row->label), state_only ? "Save state" : "Sleep");
        settings_hotkey_format(row->value, sizeof(row->value), s->sleep_keys);
        row->kind = ROW_CHEVRON;
        break;
    case SET_MENU_KEYS:
        text_copy(row->label, sizeof(row->label), state_only ? "Load state" : "Menu");
        settings_hotkey_format(row->value, sizeof(row->value), s->menu_keys);
        row->kind = ROW_CHEVRON;
        break;
    case SET_HEADING_SYSTEM:
        text_copy(row->label, sizeof(row->label), "System");
        row->kind = ROW_HEADING;
        break;
    case SET_DATETIME:
        text_copy(row->label, sizeof(row->label), "Date & Time");
        platform_clock_read(&now);
        snprintf(row->value, sizeof(row->value), "%02u-%02u %02u:%02u", now.month, now.day,
                 now.hour, now.minute);
        row->kind = ROW_CHEVRON;
        break;
    case SET_GAME_RTC:
        text_copy(row->label, sizeof(row->label), "Clock for games");
        text_copy(row->value, sizeof(row->value), s->game_rtc ? "On" : "Off");
        break;
    case SET_FAST_PATCH:
        text_copy(row->label, sizeof(row->label), "Fast patching");
        text_copy(row->value, sizeof(row->value), s->fast_patch ? "On" : "Off");
        break;
    default:
        text_copy(row->label, sizeof(row->label), "Box art in list");
        text_copy(row->value, sizeof(row->value), s->list_art ? "On" : "Off");
        break;
    }
}

static bool is_toggle(setting_item_t item)
{
    switch (item) {
    case SET_RESET_HOOK:
    case SET_SAVE_STATE_HOOK:
    case SET_SLEEP_HOOK:
    case SET_CHEATS:
    case SET_GAME_RTC:
    case SET_FAST_PATCH:
    case SET_LIST_ART:
        return true;
    default:
        return false;
    }
}

static void settings_hints(const ui_list_t *list)
{
    ui_hints(is_toggle((setting_item_t)ui_list_selected(list)) ? "A On / off|B Back"
                                                               : "A Edit|B Back");
}

void ui_settings(app_t *app)
{
    settings_t *s = &app->settings;
    settings_t original = *s;
    ui_list_t list;
    input_t input;
    unsigned selected = SET_RESET_HOOK;

    for (;;) {
        platform_set_key_repeat(LIST_REPEAT_DELAY, LIST_REPEAT_RATE);
        ui_title_bar("Settings", NULL);
        ui_list_init(&list, SET_COUNT, selected, settings_row, app, CONTENT_TOP, ROW_HEIGHT);
        ui_list_draw(&list);
        settings_hints(&list);

        bool reopen = false;
        while (!reopen) {
            platform_wait_vblank();
            ui_tick();
            platform_read_input(&input);
            ui_list_event_t event = ui_list_update(&list, &input);
            settings_hints(&list);
            setting_item_t item = (setting_item_t)ui_list_selected(&list);
            if ((input.repeated & (BTN_LEFT | BTN_RIGHT)) && is_toggle(item)) {
                event = UI_LIST_ACTIVATE; /* left / right also switch on and off */
            }
            if (event == UI_LIST_BACK) {
                if (memcmp(&original, s, sizeof(original)) != 0) {
                    app_save_settings(app);
                }
                return;
            }
            if (event != UI_LIST_ACTIVATE) {
                continue;
            }

            bool state_only = settings_save_state_only(s);
            selected = ui_list_selected(&list);
            switch ((setting_item_t)selected) {
            case SET_RESET_HOOK:
                s->reset_hook = !s->reset_hook;
                break;
            case SET_SAVE_STATE_HOOK:
                s->save_state_hook = !s->save_state_hook;
                break;
            case SET_SLEEP_HOOK:
                s->sleep_hook = !s->sleep_hook;
                break;
            case SET_CHEATS:
                s->cheats = !s->cheats;
                break;
            case SET_GAME_RTC:
                s->game_rtc = !s->game_rtc;
                break;
            case SET_FAST_PATCH:
                s->fast_patch = !s->fast_patch;
                break;
            case SET_LIST_ART:
                s->list_art = !s->list_art;
                break;
            case SET_SLEEP_KEYS:
                ui_edit_hotkey(state_only ? "Save State Hotkey" : "Sleep Hotkey",
                               state_only ? "Hold these buttons in a game to save its state."
                                          : "Hold these buttons in a game to put the GBA to sleep.",
                               s->sleep_keys);
                reopen = true;
                break;
            case SET_MENU_KEYS:
                ui_edit_hotkey(state_only ? "Load State Hotkey" : "Menu Hotkey",
                               state_only ? "Hold these buttons in a game to load its state."
                                          : "Hold these buttons in a game to return to this menu.",
                               s->menu_keys);
                reopen = true;
                break;
            case SET_DATETIME:
                ui_edit_datetime();
                reopen = true;
                break;
            default:
                break;
            }
            if (!reopen) {
                /* Toggling add-ons can rename the hotkey rows: redraw all. */
                ui_list_draw(&list);
            }
        }
    }
}

/* -------------------------------------------------------------- about -- */

void ui_about(app_t *app)
{
    char line[64];
    int y = CONTENT_TOP + 16;

    ui_title_bar("About", NULL);
    ui_clear_content();
    ui_hints("B Back");

    gfx_text(&FONT_TITLE, GFX_WIDTH / 2, y, COLOR_TEXT, "Omega Kernel", 0, ALIGN_CENTER);
    y += 20;
    snprintf(line, sizeof(line), "Version %s (%s)", KERNEL_VERSION, KERNEL_REVISION);
    gfx_text(&FONT_BODY, GFX_WIDTH / 2, y, COLOR_TEXT_MUTED, line, GFX_WIDTH - 16, ALIGN_CENTER);
    y += 16;
    snprintf(line, sizeof(line), "Cartridge firmware %u", app->fpga_version);
    gfx_text(&FONT_BODY, GFX_WIDTH / 2, y, COLOR_TEXT_MUTED, line, 0, ALIGN_CENTER);
    y += 28;
    ui_draw_paragraph(16, y, GFX_WIDTH - 32,
                      "The bar at the bottom of each screen shows what the buttons do.", COLOR_TEXT,
                      true);

    ui_wait_for(BTN_A | BTN_B);
}
