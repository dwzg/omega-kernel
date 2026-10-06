/**
 * @file gfx.h
 * @brief Drawing into the 240x160, 15-bit frame buffer (GBA video mode 3).
 *
 * Colours are 15-bit BGR (bit 0-4 red, 5-9 green, 10-14 blue). Bit 15 is
 * ignored everywhere, so bitmaps produced by grit (which sets it) can be used
 * directly.
 *
 * Portable: on the host the frame buffer is an ordinary array, which lets the
 * UI be rendered and compared in tests.
 */
#ifndef GFX_GFX_H
#define GFX_GFX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gfx/font.h"

#define GFX_WIDTH 240
#define GFX_HEIGHT 160

/** A 15-bit colour from 8-bit-per-channel components (rounded down). */
#define RGB15(r, g, b) ((uint16_t)(((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10)))

/** Ellipsis appended by gfx_text_fit(). */
#define GFX_ELLIPSIS "..."

/** Horizontal alignment for gfx_text(). */
typedef enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT } gfx_align_t;

/** @brief Pointer to the frame buffer (GFX_WIDTH * GFX_HEIGHT pixels). */
uint16_t *gfx_framebuffer(void);

/** @brief Switch the display to the bitmap mode used by the kernel. */
void gfx_init(void);

/** Rows of the off-screen buffer (see gfx_offscreen_begin()). */
#define GFX_OFFSCREEN_ROWS 32

/**
 * @brief Off-screen buffer of GFX_WIDTH x ::GFX_OFFSCREEN_ROWS pixels,
 * 32-bit aligned (provided by the platform, like gfx_framebuffer()).
 */
uint16_t *gfx_offscreen_buffer(void);

/**
 * @brief Draw screen rows [@p y, @p y + @p rows) into the off-screen buffer.
 *
 * The band starts as a copy of the screen. Until gfx_offscreen_end(), all
 * drawing goes to the buffer and is clipped to that band; coordinates stay
 * screen coordinates. Used to build a part of the screen completely before
 * showing it, so it never appears half-drawn.
 */
void gfx_offscreen_begin(int y, int rows);

/** @brief Draw to the screen again. */
void gfx_offscreen_end(void);

/** @brief Copy the band drawn off-screen last onto the screen. */
void gfx_offscreen_present(void);

/** @brief Move screen rows [@p y, @p y + @p rows) by @p dy rows (fast scrolling). */
void gfx_move_rows(int y, int rows, int dy);

/**
 * @brief Move the part [@p x, @p x + @p width) of screen rows [@p y, @p y +
 * @p rows) by @p dy rows. @p x and @p width must be even.
 */
void gfx_move_area(int x, int width, int y, int rows, int dy);

/** @brief Fill a rectangle (clipped to the screen). */
void gfx_fill(int x, int y, int w, int h, uint16_t color);

/** @brief Fill a rectangle with a vertical gradient from @p top to @p bottom. */
void gfx_fill_gradient(int x, int y, int w, int h, uint16_t top, uint16_t bottom);

/**
 * @brief Fill a rectangle with rounded corners.
 * Corner pixels are blended with what is already on screen.
 */
void gfx_fill_rounded(int x, int y, int w, int h, int radius, uint16_t color);

/** @brief One-pixel outline of a rectangle. */
void gfx_frame(int x, int y, int w, int h, uint16_t color);

/** @brief Blend a rectangle towards @p color by @p alpha (0-16 sixteenths). */
void gfx_shade(int x, int y, int w, int h, uint16_t color, unsigned alpha);

/** @brief Copy a w x h bitmap (row-major, @p stride pixels per row). */
void gfx_blit(int x, int y, int w, int h, const uint16_t *pixels, int stride);

/**
 * @brief Copy a bitmap at half size (every second pixel of every second row).
 * @p w and @p h are the size on screen; the source is twice as large.
 */
void gfx_blit_half(int x, int y, int w, int h, const uint16_t *pixels, int stride);

/** @brief Width in pixels of @p text in @p font. */
int gfx_text_width(const font_t *font, const char *text);

/**
 * @brief Shorten @p text with an ellipsis so it fits @p max_width.
 * @return @p out (always NUL-terminated).
 */
char *gfx_text_fit(const font_t *font, const char *text, int max_width, char *out, size_t out_size);

/**
 * @brief Draw text with its line box starting at (@p x, @p y).
 * @param max_width If > 0, the text is shortened with an ellipsis to fit.
 * @return The width actually drawn.
 */
int gfx_text(const font_t *font, int x, int y, uint16_t color, const char *text, int max_width,
             gfx_align_t align);

/** @brief Draw a small right-pointing chevron (7 px tall) at (@p x, @p y). */
void gfx_chevron(int x, int y, uint16_t color);

/**
 * @brief Draw a small solid triangle (7 px wide, 4 px tall) centred on @p x.
 * @param up Points up if true, down otherwise.
 */
void gfx_arrow(int x, int y, uint16_t color, bool up);

#endif /* GFX_GFX_H */
