/**
 * @file theme.h
 * @brief Colours and metrics of the interface.
 *
 * Every screen has the same three parts:
 *
 *     ┌────────────────────────────────────┐
 *     │ Title                  3/13  09:41 │  title bar (TITLE_BAR_HEIGHT)
 *     ├────────────────────────────────────┤
 *     │ list or page content               │  CONTENT_TOP .. CONTENT_BOTTOM
 *     ├────────────────────────────────────┤
 *     │ (A) Open  (B) Back  (START) Recent │  hint bar (HINT_BAR_HEIGHT)
 *     └────────────────────────────────────┘
 *
 * Text uses pixel fonts (no anti-aliasing), so it is sharp on the GBA
 * screen. See docs/user-interface.md.
 */
#ifndef UI_THEME_H
#define UI_THEME_H

#include "gfx/gfx.h"

/** @name Colours */
/**@{*/
#define COLOR_BACKGROUND RGB15(255, 255, 255)
#define COLOR_TEXT RGB15(16, 16, 24)
#define COLOR_TEXT_MUTED RGB15(104, 110, 122)
#define COLOR_TEXT_ON_SELECTION RGB15(255, 255, 255)
#define COLOR_SELECTION RGB15(40, 96, 200)
#define COLOR_RULE RGB15(208, 212, 220)
#define COLOR_BAR RGB15(32, 36, 48)         /**< Title bar background. */
#define COLOR_BAR_TEXT RGB15(255, 255, 255) /**< Title bar text. */
#define COLOR_BAR_MUTED RGB15(168, 176, 192)
#define COLOR_HINT_BAR RGB15(232, 235, 240) /**< Button hint bar background. */
#define COLOR_BUTTON RGB15(48, 54, 68)      /**< Button badges in the hint bar. */
#define COLOR_SCROLL_TRACK RGB15(232, 235, 240)
#define COLOR_SCROLL_THUMB RGB15(144, 152, 168)
#define COLOR_PLACEHOLDER RGB15(232, 235, 240)
#define COLOR_DANGER RGB15(200, 40, 40)
/**@}*/

/** @name Metrics (pixels) */
/**@{*/
#define TITLE_BAR_HEIGHT 16
#define HINT_BAR_HEIGHT 14
#define CONTENT_TOP TITLE_BAR_HEIGHT
#define CONTENT_BOTTOM (GFX_HEIGHT - HINT_BAR_HEIGHT)
#define CONTENT_HEIGHT (CONTENT_BOTTOM - CONTENT_TOP)
#define ROW_HEIGHT 16
#define ROW_PADDING 8
#define SCROLLBAR_WIDTH 3
#define LIST_ROWS (CONTENT_HEIGHT / ROW_HEIGHT) /* 8 */
/**@}*/

/** Key repeat timing for lists (frames). */
#define LIST_REPEAT_DELAY 15
#define LIST_REPEAT_RATE 4

#endif /* UI_THEME_H */
