/**
 * @file widgets.h
 * @brief Building blocks shared by all screens: title bar, button hints,
 * list, dialogs, progress display.
 */
#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include <stdbool.h>
#include <stdint.h>

#include "core/list_view.h"
#include "loader/progress.h"
#include "platform/platform.h"

/* --------------------------------------------------------------- title -- */

/** @brief Set and draw the title bar (@p title left; @p info and the clock right). */
void ui_title_bar(const char *title, const char *info);

/** @brief Call once per frame: keeps the title bar clock current. */
void ui_tick(void);

/** @brief Clear the content area between title bar and hint bar. */
void ui_clear_content(void);

/* --------------------------------------------------------------- hints -- */

/**
 * @brief Draw the button hint bar at the bottom of the screen.
 *
 * @p hints lists button/action pairs separated by '|', each "BUTTON Action":
 * `"A Open|B Back|START Recent"`. Buttons are A, B, L, R, START, SELECT, and
 * the arrows `<>` (left/right) and `^v` (up/down). The bar is only redrawn
 * when the text changes. NULL or "" leaves the bar empty.
 */
void ui_hints(const char *hints);

/* ---------------------------------------------------------------- list -- */

/** How a row is drawn. */
typedef enum {
    ROW_PLAIN,   /**< Label (and optional value on the right). */
    ROW_CHEVRON, /**< Label and ">" : opens another screen. */
    ROW_HEADING, /**< Small grey heading, not selectable. */
    ROW_CHECK    /**< Label with a check mark when @ref ui_row_t::checked. */
} ui_row_kind_t;

/** Contents of one row, filled in on demand by a ::ui_row_fn. */
typedef struct {
    char label[104];
    char value[24];
    ui_row_kind_t kind;
    bool checked;
    bool dimmed; /**< Drawn in grey (e.g. unavailable actions). */
} ui_row_t;

/** Fill @p row for item @p index (the row is zeroed beforehand). */
typedef void (*ui_row_fn)(void *ctx, unsigned index, ui_row_t *row);

/** A scrolling list filling a column of the content area. */
typedef struct {
    list_view_t view;
    ui_row_fn rows;
    void *ctx;
    int top;    /**< First pixel row of the list area. */
    int bottom; /**< End of the list area (exclusive). */
    int left;   /**< First pixel column. */
    int width;  /**< Width including the scroll bar. */
    int row_height;
    const char *empty_text; /**< Shown when the list has no rows. */
    /* Marquee state for a selected label that does not fit. */
    unsigned marquee_frames; /**< Frames since the label last rested. */
    int marquee_offset;      /**< Pixels scrolled. */
    int marquee_span;        /**< Label width + gap, 0 if the label fits. */
    bool present_pending;    /**< A drawn row waits for the vertical blank. */
} ui_list_t;

/** Result of ui_list_update(). */
typedef enum {
    UI_LIST_IDLE,     /**< Nothing for the screen to do. */
    UI_LIST_ACTIVATE, /**< A was pressed on the selected row. */
    UI_LIST_BACK      /**< B was pressed. */
} ui_list_event_t;

/**
 * @brief Set up a full-width list from @p top to the hint bar.
 * @param selected Initially selected row.
 */
void ui_list_init(ui_list_t *list, unsigned count, unsigned selected, ui_row_fn rows, void *ctx,
                  int top, int row_height);

/** @brief Restrict the list to the column [@p left, @p left + @p width). */
void ui_list_set_column(ui_list_t *list, int left, int width);

/** @brief Draw the whole list area. */
void ui_list_draw(ui_list_t *list);

/** @brief Redraw only the selected row (after its contents changed). */
void ui_list_redraw_selected(ui_list_t *list);

/**
 * @brief Handle navigation keys (up/down, L/R page) and redraw as needed.
 * @return What the screen should react to.
 */
ui_list_event_t ui_list_update(ui_list_t *list, const input_t *input);

/** @brief The selected row index. */
static inline unsigned ui_list_selected(const ui_list_t *list)
{
    return list->view.selected;
}

/* ------------------------------------------------------------- dialogs -- */

/** @brief Show a message and wait for A or B. */
void ui_message(const char *title, const char *text);

/**
 * @brief Ask a yes/no question: A does @p action, B cancels.
 * @param action What A does (e.g. "Delete"), shown in the hint bar.
 * @return true if the user pressed A.
 */
bool ui_confirm(const char *title, const char *text, const char *action);

/** @brief Draw word-wrapped text in a column. @return y after the last line. */
int ui_draw_paragraph(int x, int y, int width, const char *text, uint16_t color, bool center);

/* ------------------------------------------------------------ progress -- */

/** @brief Show a progress screen with @p title and return its reporter. */
const progress_t *ui_progress_begin(const char *title);

/** @brief Wait for one of @p buttons to be pressed. @return the button pressed. */
uint16_t ui_wait_for(uint16_t buttons);

#endif /* UI_WIDGETS_H */
