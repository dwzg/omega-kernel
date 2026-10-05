/**
 * @file list_view.h
 * @brief Cursor and scroll position of a vertical list.
 *
 * Holds no items: it only tracks which index is selected and which index is
 * at the top of the visible window, and moves them consistently. Optional
 * "selectable" callbacks let lists contain non-selectable rows (headings).
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_LIST_VIEW_H
#define CORE_LIST_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Returns whether row @p index can be selected. */
typedef bool (*list_selectable_fn)(void *ctx, unsigned index);

typedef struct {
    unsigned count;                /**< Number of rows. */
    unsigned visible;              /**< Rows that fit on screen. */
    unsigned selected;             /**< Selected row (meaningless when count == 0). */
    unsigned top;                  /**< First visible row. */
    list_selectable_fn selectable; /**< NULL = every row is selectable. */
    void *ctx;                     /**< Passed to @ref selectable. */
} list_view_t;

/**
 * @brief Initialise a view and select @p selected (clamped, moved to the
 * nearest selectable row) with the selection visible.
 */
void list_view_init(list_view_t *lv, unsigned count, unsigned visible, unsigned selected);

/** @brief Set the selectable-row callback, then re-validate the selection. */
void list_view_set_selectable(list_view_t *lv, list_selectable_fn fn, void *ctx);

/**
 * @brief Move the selection by @p delta rows (skipping non-selectable rows),
 * stopping at either end.
 * @return true if the selection or scroll position changed.
 */
bool list_view_move(list_view_t *lv, int delta);

/** @brief Move by a whole page (@p direction = -1 or +1). */
bool list_view_page(list_view_t *lv, int direction);

/** @brief Restore a remembered position (e.g. after returning from a folder). */
void list_view_restore(list_view_t *lv, unsigned selected, unsigned top);

#endif /* CORE_LIST_VIEW_H */
