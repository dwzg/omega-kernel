/**
 * @file list_view.c
 * @brief List cursor model. See list_view.h.
 */
#include "core/list_view.h"

static bool is_selectable(const list_view_t *lv, unsigned index)
{
    return lv->selectable == NULL || lv->selectable(lv->ctx, index);
}

/** Scroll so that the selection is inside the visible window. */
static void keep_visible(list_view_t *lv)
{
    if (lv->visible == 0) {
        lv->top = 0;
        return;
    }
    if (lv->selected < lv->top) {
        lv->top = lv->selected;
    } else if (lv->selected >= lv->top + lv->visible) {
        lv->top = lv->selected - lv->visible + 1;
    }
    /* Do not leave empty rows at the bottom when the list could fill them. */
    unsigned max_top = lv->count > lv->visible ? lv->count - lv->visible : 0;
    if (lv->top > max_top) {
        lv->top = max_top;
    }
}

/** Move the selection to the nearest selectable row (forward first). */
static void snap_to_selectable(list_view_t *lv)
{
    if (lv->count == 0 || is_selectable(lv, lv->selected)) {
        return;
    }
    for (unsigned i = lv->selected + 1; i < lv->count; i++) {
        if (is_selectable(lv, i)) {
            lv->selected = i;
            return;
        }
    }
    for (unsigned i = lv->selected; i-- > 0;) {
        if (is_selectable(lv, i)) {
            lv->selected = i;
            return;
        }
    }
}

void list_view_init(list_view_t *lv, unsigned count, unsigned visible, unsigned selected)
{
    lv->count = count;
    lv->visible = visible;
    lv->selected = (count == 0) ? 0 : (selected < count ? selected : count - 1);
    lv->top = 0;
    lv->selectable = NULL;
    lv->ctx = NULL;
    keep_visible(lv);
}

void list_view_set_selectable(list_view_t *lv, list_selectable_fn fn, void *ctx)
{
    lv->selectable = fn;
    lv->ctx = ctx;
    snap_to_selectable(lv);
    keep_visible(lv);
}

bool list_view_move(list_view_t *lv, int delta)
{
    if (lv->count == 0 || delta == 0) {
        return false;
    }
    unsigned old_selected = lv->selected;
    unsigned old_top = lv->top;
    int step = delta > 0 ? 1 : -1;
    int remaining = delta > 0 ? delta : -delta;
    unsigned candidate = lv->selected;

    while (remaining > 0) {
        if ((step < 0 && candidate == 0) || (step > 0 && candidate + 1 >= lv->count)) {
            break;
        }
        candidate = (unsigned)((int)candidate + step);
        if (is_selectable(lv, candidate)) {
            lv->selected = candidate;
            remaining--;
        }
    }
    keep_visible(lv);
    /* Show a heading that sits just above the first selectable row. */
    if (lv->top > 0 && lv->selected == lv->top && !is_selectable(lv, lv->top - 1)) {
        lv->top--;
    }
    return lv->selected != old_selected || lv->top != old_top;
}

bool list_view_page(list_view_t *lv, int direction)
{
    int rows = lv->visible > 1 ? (int)lv->visible - 1 : 1;
    return list_view_move(lv, direction * rows);
}

void list_view_restore(list_view_t *lv, unsigned selected, unsigned top)
{
    if (lv->count == 0) {
        return;
    }
    lv->selected = selected < lv->count ? selected : lv->count - 1;
    lv->top = top;
    snap_to_selectable(lv);
    keep_visible(lv);
}
