/**
 * @file widgets.c
 * @brief Shared UI building blocks. See widgets.h.
 *
 * Anything that changes while it is visible (title bar, hint bar, list rows)
 * is built off-screen and copied in one go, so it never shows half-drawn.
 */
#include "ui/widgets.h"

#include <stdio.h>
#include <string.h>

#include "ui/theme.h"

/* --------------------------------------------------------------- title -- */

static char s_title[64];
static char s_title_info[24];
static uint8_t s_shown_minute = 0xFF;
static unsigned s_frames;

static void draw_title_bar(void)
{
    datetime_t now;
    char clock[8];

    platform_clock_read(&now);
    s_shown_minute = now.minute;
    snprintf(clock, sizeof(clock), "%02u:%02u", now.hour, now.minute);

    gfx_offscreen_begin(0, TITLE_BAR_HEIGHT);
    gfx_fill(0, 0, GFX_WIDTH, TITLE_BAR_HEIGHT, COLOR_BAR);
    int right = GFX_WIDTH - 6;
    right -= gfx_text(&FONT_SMALL, right, 2, COLOR_BAR_TEXT, clock, 0, ALIGN_RIGHT) + 8;
    if (s_title_info[0]) {
        right -= gfx_text(&FONT_SMALL, right, 2, COLOR_BAR_MUTED, s_title_info, 0, ALIGN_RIGHT) + 8;
    }
    gfx_text(&FONT_TITLE, 6, 0, COLOR_BAR_TEXT, s_title, right - 6, ALIGN_LEFT);
    gfx_offscreen_end();
    gfx_offscreen_present();
}

void ui_title_bar(const char *title, const char *info)
{
    snprintf(s_title, sizeof(s_title), "%s", title);
    snprintf(s_title_info, sizeof(s_title_info), "%s", info ? info : "");
    draw_title_bar();
}

void ui_tick(void)
{
    /* Reading the RTC takes a moment; twice a second is plenty. */
    if (++s_frames % 30 != 0) {
        return;
    }
    datetime_t now;
    platform_clock_read(&now);
    if (now.minute != s_shown_minute) {
        draw_title_bar();
    }
}

void ui_clear_content(void)
{
    gfx_fill(0, CONTENT_TOP, GFX_WIDTH, CONTENT_HEIGHT, COLOR_BACKGROUND);
}

/* --------------------------------------------------------------- hints -- */

#define BADGE_HEIGHT 11

static char s_hints[96];
static bool s_hints_drawn;

/** Draw a button badge at @p x; @return its width. */
static int draw_badge(int x, int y, const char *button)
{
    bool arrows_lr = strcmp(button, "<>") == 0;
    bool arrows_ud = strcmp(button, "^v") == 0;
    int width = arrows_lr ? 15 : arrows_ud ? 11 : gfx_text_width(&FONT_SMALL, button) + 7;

    gfx_fill(x, y, width, BADGE_HEIGHT, COLOR_BUTTON);
    /* Round the corners by one pixel. */
    gfx_fill(x, y, 1, 1, COLOR_HINT_BAR);
    gfx_fill(x + width - 1, y, 1, 1, COLOR_HINT_BAR);
    gfx_fill(x, y + BADGE_HEIGHT - 1, 1, 1, COLOR_HINT_BAR);
    gfx_fill(x + width - 1, y + BADGE_HEIGHT - 1, 1, 1, COLOR_HINT_BAR);

    if (arrows_lr) {
        for (int i = 0; i < 4; i++) {
            gfx_fill(x + 3 + i, y + 5 - i, 1, 2 * i + 1, COLOR_BACKGROUND);  /* < */
            gfx_fill(x + 11 - i, y + 5 - i, 1, 2 * i + 1, COLOR_BACKGROUND); /* > */
        }
    } else if (arrows_ud) {
        /* Up arrow above a down arrow. */
        gfx_arrow(x + 5, y + 1, COLOR_BACKGROUND, true);
        gfx_arrow(x + 5, y + 6, COLOR_BACKGROUND, false);
    } else {
        gfx_text(&FONT_SMALL, x + 4, y - 1, COLOR_BACKGROUND, button, 0, ALIGN_LEFT);
    }
    return width;
}

static void draw_hint_bar(void)
{
    int top = GFX_HEIGHT - HINT_BAR_HEIGHT;
    int x = 6;
    const char *p = s_hints;

    gfx_offscreen_begin(top, HINT_BAR_HEIGHT);
    gfx_fill(0, top, GFX_WIDTH, HINT_BAR_HEIGHT, COLOR_HINT_BAR);
    gfx_fill(0, top, GFX_WIDTH, 1, COLOR_RULE);
    while (*p) {
        char button[8];
        char action[32];
        size_t n = 0;
        while (*p == ' ') {
            p++;
        }
        while (*p && *p != ' ' && *p != '|' && n + 1 < sizeof(button)) {
            button[n++] = *p++;
        }
        button[n] = '\0';
        while (*p == ' ') {
            p++;
        }
        n = 0;
        while (*p && *p != '|' && n + 1 < sizeof(action)) {
            action[n++] = *p++;
        }
        action[n] = '\0';
        if (*p == '|') {
            p++;
        }
        if (button[0]) {
            x += draw_badge(x, top + 2, button) + 4;
            x += gfx_text(&FONT_SMALL, x, top + 1, COLOR_TEXT, action, GFX_WIDTH - x - 4,
                          ALIGN_LEFT) +
                 10;
        }
    }
    gfx_offscreen_end();
    gfx_offscreen_present();
    s_hints_drawn = true;
}

void ui_hints(const char *hints)
{
    if (!hints) {
        hints = "";
    }
    if (s_hints_drawn && strcmp(hints, s_hints) == 0) {
        return;
    }
    snprintf(s_hints, sizeof(s_hints), "%s", hints);
    draw_hint_bar();
}

/* ---------------------------------------------------------------- list -- */

/** Frames a long selected label rests before (and between) scrolling. */
#define MARQUEE_PAUSE 60
/** Gap between the end of the label and its repeat. */
#define MARQUEE_GAP 24

static void get_row(const ui_list_t *list, unsigned index, ui_row_t *row)
{
    memset(row, 0, sizeof(*row));
    list->rows(list->ctx, index, row);
}

static bool row_selectable(void *ctx, unsigned index)
{
    ui_row_t row;
    get_row(ctx, index, &row);
    return row.kind != ROW_HEADING;
}

void ui_list_init(ui_list_t *list, unsigned count, unsigned selected, ui_row_fn rows, void *ctx,
                  int top, int row_height)
{
    memset(list, 0, sizeof(*list));
    list->rows = rows;
    list->ctx = ctx;
    list->top = top;
    list->bottom = CONTENT_BOTTOM;
    list->left = 0;
    list->width = GFX_WIDTH;
    list->row_height = row_height;
    list_view_init(&list->view, count, (unsigned)((list->bottom - top) / row_height), selected);
    list_view_set_selectable(&list->view, row_selectable, list);
}

void ui_list_set_column(ui_list_t *list, int left, int width)
{
    list->left = left;
    list->width = width;
}

static bool has_scrollbar(const ui_list_t *list)
{
    return list->view.count > list->view.visible;
}

/** Width available to row contents (without the scroll bar). */
static int row_width(const ui_list_t *list)
{
    return list->width - (has_scrollbar(list) ? SCROLLBAR_WIDTH + 1 : 0);
}

static void draw_scrollbar(const ui_list_t *list)
{
    const list_view_t *v = &list->view;
    if (!has_scrollbar(list)) {
        return;
    }
    int x = list->left + list->width - SCROLLBAR_WIDTH;
    int height = list->bottom - list->top;
    int thumb = height * (int)v->visible / (int)v->count;
    if (thumb < 8) {
        thumb = 8;
    }
    int travel = height - thumb;
    int pos = travel * (int)v->top / (int)(v->count - v->visible);
    gfx_fill(x - 1, list->top, 1, height, COLOR_BACKGROUND);
    gfx_fill(x, list->top, SCROLLBAR_WIDTH, pos, COLOR_SCROLL_TRACK);
    gfx_fill(x, list->top + pos, SCROLLBAR_WIDTH, thumb, COLOR_SCROLL_THUMB);
    gfx_fill(x, list->top + pos + thumb, SCROLLBAR_WIDTH, height - pos - thumb, COLOR_SCROLL_TRACK);
}

/** Draw a check mark ending left of @p right, centred on @p cy. */
static void draw_check(int right, int cy, uint16_t color)
{
    int cx = right - 9;
    for (int i = 0; i < 3; i++) {
        gfx_fill(cx + i, cy + i - 1, 2, 2, color);
    }
    for (int i = 0; i < 5; i++) {
        gfx_fill(cx + 3 + i, cy + 1 - i, 2, 2, color);
    }
}

/**
 * Draw row @p index. The row is built off-screen and copied in one go, so it
 * never shows half-drawn. With @p defer, the copy is left for
 * flush_pending() at the next vertical blank (used by the marquee).
 */
static void draw_row(ui_list_t *list, unsigned index, bool defer)
{
    ui_row_t row;
    int h = list->row_height;
    int y = list->top + (int)(index - list->view.top) * h;
    int x0 = list->left;
    int width = row_width(list);
    bool selected = index == list->view.selected;

    get_row(list, index, &row);

    gfx_offscreen_begin(y, h);
    gfx_fill(x0, y, width, h, selected ? COLOR_SELECTION : COLOR_BACKGROUND);

    uint16_t fg = selected ? COLOR_TEXT_ON_SELECTION : (row.dimmed ? COLOR_TEXT_MUTED : COLOR_TEXT);
    uint16_t muted = selected ? COLOR_TEXT_ON_SELECTION : COLOR_TEXT_MUTED;
    int text_y = y + (h - font_line_height(&FONT_BODY)) / 2;
    int small_y = y + (h - font_line_height(&FONT_SMALL)) / 2 + 1;
    int right = x0 + width - ROW_PADDING;

    if (row.kind == ROW_HEADING) {
        gfx_text(&FONT_SMALL, x0 + ROW_PADDING, y + h - font_line_height(&FONT_SMALL),
                 COLOR_TEXT_MUTED, row.label, width - 2 * ROW_PADDING, ALIGN_LEFT);
    } else {
        /* Right-hand items first: chevron or check, then the value. */
        if (row.kind == ROW_CHEVRON) {
            gfx_chevron(right - 5, y + (h - 7) / 2, fg);
            right -= 12;
        } else if (row.kind == ROW_CHECK) {
            if (row.checked) {
                draw_check(right, y + h / 2, fg);
            }
            right -= 14;
        }
        if (row.value[0]) {
            right -= gfx_text(&FONT_SMALL, right, small_y, muted, row.value, 0, ALIGN_RIGHT) + 8;
        }

        int label_x = x0 + ROW_PADDING;
        int max_label = right - label_x;
        int full = gfx_text_width(&FONT_BODY, row.label);
        if (selected && full > max_label) {
            /* Too long: scroll it through its column (ui_list_update()). */
            list->marquee_span = full + MARQUEE_GAP;
            int offset = list->marquee_offset % list->marquee_span;
            gfx_text(&FONT_BODY, label_x - offset, text_y, fg, row.label, 0, ALIGN_LEFT);
            if (offset > 0) {
                gfx_text(&FONT_BODY, label_x - offset + list->marquee_span, text_y, fg, row.label,
                         0, ALIGN_LEFT);
            }
            /* Repaint both sides of the column, then the right-hand items. */
            gfx_fill(x0, y, ROW_PADDING, h, COLOR_SELECTION);
            gfx_fill(right, y, x0 + width - right, h, COLOR_SELECTION);
            right = x0 + width - ROW_PADDING;
            if (row.kind == ROW_CHEVRON) {
                gfx_chevron(right - 5, y + (h - 7) / 2, fg);
                right -= 12;
            } else if (row.kind == ROW_CHECK) {
                if (row.checked) {
                    draw_check(right, y + h / 2, fg);
                }
                right -= 14;
            }
            if (row.value[0]) {
                gfx_text(&FONT_SMALL, right, small_y, muted, row.value, 0, ALIGN_RIGHT);
            }
        } else {
            if (selected) {
                list->marquee_span = 0;
            }
            gfx_text(&FONT_BODY, label_x, text_y, fg, row.label, max_label, ALIGN_LEFT);
        }
    }
    draw_scrollbar(list);
    gfx_offscreen_end();

    if (defer) {
        list->present_pending = true;
    } else {
        list->present_pending = false;
        gfx_offscreen_present();
    }
}

/** Show a row that draw_row() left for the vertical blank. */
static void flush_pending(ui_list_t *list)
{
    if (list->present_pending) {
        list->present_pending = false;
        gfx_offscreen_present();
    }
}

/** Number of rows currently shown. */
static unsigned shown_rows(const ui_list_t *list)
{
    unsigned left = list->view.count - list->view.top;
    return left < list->view.visible ? left : list->view.visible;
}

void ui_list_draw(ui_list_t *list)
{
    int area = list->bottom - list->top;
    list->present_pending = false;
    if (list->view.count == 0) {
        gfx_fill(list->left, list->top, list->width, area, COLOR_BACKGROUND);
        if (list->empty_text) {
            ui_draw_paragraph(list->left + 16, list->top + 40, list->width - 32, list->empty_text,
                              COLOR_TEXT_MUTED, true);
        }
        return;
    }
    unsigned rows = shown_rows(list);
    for (unsigned i = 0; i < rows; i++) {
        draw_row(list, list->view.top + i, false);
    }
    int end = list->top + (int)rows * list->row_height;
    gfx_fill(list->left, end, row_width(list), list->bottom - end, COLOR_BACKGROUND);
    draw_scrollbar(list);
}

void ui_list_redraw_selected(ui_list_t *list)
{
    if (list->view.count > 0) {
        draw_row(list, list->view.selected, false);
    }
}

/**
 * Redraw after the list scrolled by @p delta rows (|delta| < visible rows):
 * move the rows that stay visible, then draw only the ones that came into
 * view and the two whose selection changed. Only for full-width lists, since
 * whole screen rows are moved.
 */
static void scroll_rows(ui_list_t *list, int delta, unsigned old_selected)
{
    int h = list->row_height;
    int visible = (int)list->view.visible;
    int keep = visible - (delta > 0 ? delta : -delta);

    if (delta > 0) {
        gfx_move_rows(list->top + delta * h, keep * h, -delta * h);
    } else {
        gfx_move_rows(list->top, keep * h, -delta * h);
    }
    unsigned first_new = delta > 0 ? list->view.top + (unsigned)keep : list->view.top;
    for (unsigned i = 0; i < (unsigned)(visible - keep); i++) {
        if (first_new + i < list->view.count) {
            draw_row(list, first_new + i, false);
        }
    }
    if (old_selected >= list->view.top && old_selected < list->view.top + list->view.visible) {
        draw_row(list, old_selected, false);
    }
    draw_row(list, list->view.selected, false);
    draw_scrollbar(list); /* the moved rows carry the old thumb position */
}

ui_list_event_t ui_list_update(ui_list_t *list, const input_t *input)
{
    unsigned old_selected = list->view.selected;
    unsigned old_top = list->view.top;
    bool moved = false;

    /* We are at the start of a vertical blank: show the marquee step that
     * was prepared last frame. */
    flush_pending(list);

    if (input->repeated & BTN_DOWN) {
        moved = list_view_move(&list->view, 1);
    } else if (input->repeated & BTN_UP) {
        moved = list_view_move(&list->view, -1);
    } else if (input->repeated & BTN_R) {
        moved = list_view_page(&list->view, 1);
    } else if (input->repeated & BTN_L) {
        moved = list_view_page(&list->view, -1);
    }

    if (moved) {
        list->marquee_frames = 0;
        list->marquee_offset = 0;
        int delta = (int)list->view.top - (int)old_top;
        int limit = (int)list->view.visible;
        bool full_width = list->left == 0 && list->width == GFX_WIDTH;
        if (delta == 0) {
            draw_row(list, old_selected, false);
            draw_row(list, list->view.selected, false);
        } else if (full_width && delta > -limit && delta < limit &&
                   shown_rows(list) == list->view.visible) {
            scroll_rows(list, delta, old_selected);
        } else {
            ui_list_draw(list);
        }
    } else if (list->view.count > 0 && list->marquee_span > 0 &&
               ++list->marquee_frames > MARQUEE_PAUSE) {
        /* One pixel per frame, resting at the start of every round. */
        list->marquee_offset++;
        if (list->marquee_offset % list->marquee_span == 0) {
            list->marquee_offset = 0;
            list->marquee_frames = 0;
        }
        draw_row(list, list->view.selected, true);
    }

    if (input->pressed & BTN_A && list->view.count > 0) {
        return UI_LIST_ACTIVATE;
    }
    if (input->pressed & BTN_B) {
        return UI_LIST_BACK;
    }
    return UI_LIST_IDLE;
}

/* ------------------------------------------------------------- dialogs -- */

/** Lay out word-wrapped text; draws it only if @p draw. @return y after the last line. */
static int layout_paragraph(int x, int y, int width, const char *text, uint16_t color, bool center,
                            bool draw)
{
    char line[64];
    const char *p = text;
    int line_height = font_line_height(&FONT_BODY);

    while (*p) {
        /* Take as many words as fit. */
        size_t len = 0;
        size_t best = 0;
        while (p[len] && p[len] != '\n') {
            size_t next = len;
            while (p[next] == ' ') {
                next++;
            }
            while (p[next] && p[next] != ' ' && p[next] != '\n') {
                next++;
            }
            if (next >= sizeof(line)) {
                break;
            }
            memcpy(line, p, next);
            line[next] = '\0';
            if (gfx_text_width(&FONT_BODY, line) > width && best > 0) {
                break;
            }
            best = next;
            len = next;
        }
        if (best == 0) {
            best = len > 0 ? len : 1; /* a single very long word */
        }
        memcpy(line, p, best);
        line[best] = '\0';
        if (draw) {
            gfx_text(&FONT_BODY, center ? x + width / 2 : x, y, color, line, width,
                     center ? ALIGN_CENTER : ALIGN_LEFT);
        }
        y += line_height;
        p += best;
        while (*p == ' ' || *p == '\n') {
            p++;
        }
    }
    return y;
}

int ui_draw_paragraph(int x, int y, int width, const char *text, uint16_t color, bool center)
{
    return layout_paragraph(x, y, width, text, color, center, true);
}

uint16_t ui_wait_for(uint16_t buttons)
{
    input_t input;
    for (;;) {
        platform_wait_vblank();
        ui_tick();
        platform_read_input(&input);
        uint16_t hit = input.pressed & buttons;
        if (hit) {
            return hit;
        }
    }
}

/** Title, then @p text centred in the content area. */
static void draw_dialog(const char *title, const char *text)
{
    ui_title_bar(title, NULL);
    ui_clear_content();
    int height = layout_paragraph(16, 0, GFX_WIDTH - 32, text, 0, true, false);
    int y = CONTENT_TOP + (CONTENT_HEIGHT - height) / 2;
    ui_draw_paragraph(16, y, GFX_WIDTH - 32, text, COLOR_TEXT, true);
}

void ui_message(const char *title, const char *text)
{
    draw_dialog(title, text);
    ui_hints("A OK");
    ui_wait_for(BTN_A | BTN_B);
}

bool ui_confirm(const char *title, const char *text, const char *action)
{
    char hints[48];
    draw_dialog(title, text);
    snprintf(hints, sizeof(hints), "A %s|B Cancel", action);
    ui_hints(hints);
    return ui_wait_for(BTN_A | BTN_B) == BTN_A;
}

/* ------------------------------------------------------------ progress -- */

#define PROGRESS_X 30
#define PROGRESS_Y 88
#define PROGRESS_W (GFX_WIDTH - 2 * PROGRESS_X)
#define PROGRESS_H 8

static void progress_status_cb(void *ctx, const char *text)
{
    (void)ctx;
    gfx_offscreen_begin(56, 20);
    gfx_fill(0, 56, GFX_WIDTH, 20, COLOR_BACKGROUND);
    gfx_text(&FONT_BODY, GFX_WIDTH / 2, 58, COLOR_TEXT, text, GFX_WIDTH - 32, ALIGN_CENTER);
    gfx_offscreen_end();
    gfx_offscreen_present();
}

static void progress_advance_cb(void *ctx, uint32_t done, uint32_t total)
{
    (void)ctx;
    int filled = total ? (int)((uint64_t)(PROGRESS_W - 2) * done / total) : 0;
    char percent[8];
    snprintf(percent, sizeof(percent), "%u%%",
             total ? (unsigned)((uint64_t)100 * done / total) : 0);

    gfx_offscreen_begin(PROGRESS_Y, 28);
    gfx_fill(0, PROGRESS_Y, GFX_WIDTH, 28, COLOR_BACKGROUND);
    gfx_frame(PROGRESS_X, PROGRESS_Y, PROGRESS_W, PROGRESS_H, COLOR_SCROLL_THUMB);
    gfx_fill(PROGRESS_X + 1, PROGRESS_Y + 1, filled, PROGRESS_H - 2, COLOR_SELECTION);
    gfx_text(&FONT_SMALL, GFX_WIDTH / 2, PROGRESS_Y + 13, COLOR_TEXT_MUTED, percent, 0,
             ALIGN_CENTER);
    gfx_offscreen_end();
    gfx_offscreen_present();
}

static const progress_t PROGRESS = {progress_status_cb, progress_advance_cb, NULL};

const progress_t *ui_progress_begin(const char *title)
{
    ui_title_bar(title, NULL);
    ui_clear_content();
    ui_hints("");
    return &PROGRESS;
}
