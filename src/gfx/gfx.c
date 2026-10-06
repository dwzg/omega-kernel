/**
 * @file gfx.c
 * @brief Frame buffer drawing. See gfx.h.
 */
#include "gfx/gfx.h"

#include <string.h>

#include "core/utf8.h"
#include "platform/attributes.h"

/** Hot drawing loops run from IWRAM on the GBA (32-bit bus, no wait states). */
#define GFX_FAST PLATFORM_FAST_CODE

/** Blend @p dst towards @p src; @p weight in 0..256. */
static inline uint16_t blend(uint16_t dst, uint16_t src, int weight)
{
    int dr = dst & 31, dg = (dst >> 5) & 31, db = (dst >> 10) & 31;
    int sr = src & 31, sg = (src >> 5) & 31, sb = (src >> 10) & 31;
    dr += ((sr - dr) * weight) >> 8;
    dg += ((sg - dg) * weight) >> 8;
    db += ((sb - db) * weight) >> 8;
    return (uint16_t)(dr | (dg << 5) | (db << 10));
}

/*
 * Drawing target. Normally the screen; between gfx_offscreen_begin() and
 * gfx_offscreen_end() a band of screen rows [s_top, s_bottom) is redirected
 * to the off-screen buffer. Coordinates are always screen coordinates.
 */
static uint16_t *s_base; /* pixel (0, s_top) of the target; NULL = not set up */
static int s_top;
static int s_bottom = GFX_HEIGHT;
/* Band of the last off-screen drawing, for gfx_offscreen_present(). */
static int s_offscreen_top;
static int s_offscreen_rows;

/** Start of row @p y of the current target (y must be inside it). */
static inline __attribute__((always_inline)) uint16_t *target_row(int y)
{
    return s_base + (y - s_top) * GFX_WIDTH;
}

static inline __attribute__((always_inline)) void use_screen(void)
{
    s_base = gfx_framebuffer();
    s_top = 0;
    s_bottom = GFX_HEIGHT;
}

/** Copy @p words 32-bit words, correctly for overlapping areas. */
GFX_FAST static void copy_words(uint32_t *dst, const uint32_t *src, unsigned words)
{
    if (dst < src) {
        for (unsigned i = 0; i < words; i++) {
            dst[i] = src[i];
        }
    } else {
        while (words-- > 0) {
            dst[words] = src[words];
        }
    }
}

void gfx_offscreen_begin(int y, int rows)
{
    if (rows > GFX_OFFSCREEN_ROWS) {
        rows = GFX_OFFSCREEN_ROWS;
    }
    s_base = gfx_offscreen_buffer();
    s_top = y;
    s_bottom = y + rows;
    s_offscreen_top = y;
    s_offscreen_rows = rows;

    /* Start from what is on screen, so pixels nobody draws stay unchanged. */
    int first = y < 0 ? 0 : y;
    int last = y + rows > GFX_HEIGHT ? GFX_HEIGHT : y + rows;
    if (last > first) {
        copy_words((uint32_t *)(s_base + (first - y) * GFX_WIDTH),
                   (const uint32_t *)(gfx_framebuffer() + first * GFX_WIDTH),
                   (unsigned)((last - first) * GFX_WIDTH / 2));
    }
}

void gfx_offscreen_end(void)
{
    use_screen();
}

void gfx_offscreen_present(void)
{
    int y = s_offscreen_top;
    int rows = s_offscreen_rows;
    if (y < 0) {
        rows += y;
        y = 0;
    }
    if (y + rows > GFX_HEIGHT) {
        rows = GFX_HEIGHT - y;
    }
    if (rows <= 0) {
        return;
    }
    const uint16_t *src = gfx_offscreen_buffer() + (y - s_offscreen_top) * GFX_WIDTH;
    copy_words((uint32_t *)(gfx_framebuffer() + y * GFX_WIDTH), (const uint32_t *)src,
               (unsigned)(rows * GFX_WIDTH / 2));
}

void gfx_move_rows(int y, int rows, int dy)
{
    if (y < 0 || rows <= 0 || y + rows > GFX_HEIGHT || y + dy < 0 || y + dy + rows > GFX_HEIGHT) {
        return;
    }
    uint16_t *fb = gfx_framebuffer();
    copy_words((uint32_t *)(fb + (y + dy) * GFX_WIDTH), (const uint32_t *)(fb + y * GFX_WIDTH),
               (unsigned)(rows * GFX_WIDTH / 2));
}

void gfx_move_area(int x, int width, int y, int rows, int dy)
{
    if (x == 0 && width == GFX_WIDTH) {
        gfx_move_rows(y, rows, dy);
        return;
    }
    if ((x | width) & 1 || x < 0 || width <= 0 || x + width > GFX_WIDTH || y < 0 || rows <= 0 ||
        y + rows > GFX_HEIGHT || y + dy < 0 || y + dy + rows > GFX_HEIGHT) {
        return;
    }
    uint16_t *fb = gfx_framebuffer();
    /* Line by line, in the order that never overwrites a line still to move. */
    for (int i = 0; i < rows; i++) {
        int line = dy < 0 ? y + i : y + rows - 1 - i;
        copy_words((uint32_t *)(fb + (line + dy) * GFX_WIDTH + x),
                   (const uint32_t *)(fb + line * GFX_WIDTH + x), (unsigned)width / 2);
    }
}

/** Clip a rectangle to the target. @return false if nothing is left. */
static inline __attribute__((always_inline)) bool clip(int *x, int *y, int *w, int *h)
{
    if (*x < 0) {
        *w += *x;
        *x = 0;
    }
    if (*y < s_top) {
        *h -= s_top - *y;
        *y = s_top;
    }
    if (*x + *w > GFX_WIDTH) {
        *w = GFX_WIDTH - *x;
    }
    if (*y + *h > s_bottom) {
        *h = s_bottom - *y;
    }
    return *w > 0 && *h > 0;
}

GFX_FAST void gfx_fill(int x, int y, int w, int h, uint16_t color)
{
    if (!s_base) {
        use_screen();
    }
    if (!clip(&x, &y, &w, &h)) {
        return;
    }
    uint16_t *row = target_row(y) + x;
    for (int j = 0; j < h; j++, row += GFX_WIDTH) {
        for (int i = 0; i < w; i++) {
            row[i] = color;
        }
    }
}

void gfx_fill_gradient(int x, int y, int w, int h, uint16_t top, uint16_t bottom)
{
    for (int j = 0; j < h; j++) {
        int weight = h > 1 ? (j * 256) / (h - 1) : 0;
        gfx_fill(x, y + j, w, 1, blend(top, bottom, weight));
    }
}

void gfx_fill_rounded(int x, int y, int w, int h, int radius, uint16_t color)
{
    /* Coverage of the corner pixels for radius 1-3, outermost row first. */
    static const uint8_t CORNER[3][3][3] = {
        {{8, 0, 0}},
        {{0, 10, 0}, {10, 16, 0}},
        {{0, 6, 14}, {6, 16, 16}, {14, 16, 16}},
    };
    if (radius < 1) {
        gfx_fill(x, y, w, h, color);
        return;
    }
    if (radius > 3) {
        radius = 3;
    }
    gfx_fill(x, y + radius, w, h - 2 * radius, color);
    gfx_fill(x + radius, y, w - 2 * radius, radius, color);
    gfx_fill(x + radius, y + h - radius, w - 2 * radius, radius, color);

    const uint8_t(*c)[3] = CORNER[radius - 1];
    for (int j = 0; j < radius; j++) {
        for (int i = 0; i < radius; i++) {
            int weight = c[j][i] * 16; /* sixteenths -> 0..256 */
            const int xs[2] = {x + i, x + w - 1 - i};
            const int ys[2] = {y + j, y + h - 1 - j};
            for (int a = 0; a < 2; a++) {
                for (int b = 0; b < 2; b++) {
                    int px = xs[a], py = ys[b];
                    if (px >= 0 && px < GFX_WIDTH && py >= s_top && py < s_bottom) {
                        uint16_t *p = target_row(py) + px;
                        *p = blend(*p, color, weight);
                    }
                }
            }
        }
    }
}

void gfx_frame(int x, int y, int w, int h, uint16_t color)
{
    gfx_fill(x, y, w, 1, color);
    gfx_fill(x, y + h - 1, w, 1, color);
    gfx_fill(x, y, 1, h, color);
    gfx_fill(x + w - 1, y, 1, h, color);
}

void gfx_shade(int x, int y, int w, int h, uint16_t color, unsigned alpha)
{
    if (!clip(&x, &y, &w, &h)) {
        return;
    }
    int weight = (int)(alpha > 16 ? 16 : alpha) * 16;
    if (!s_base) {
        use_screen();
    }
    uint16_t *row = target_row(y) + x;
    for (int j = 0; j < h; j++, row += GFX_WIDTH) {
        for (int i = 0; i < w; i++) {
            row[i] = blend(row[i], color, weight);
        }
    }
}

GFX_FAST void gfx_blit(int x, int y, int w, int h, const uint16_t *pixels, int stride)
{
    if (!s_base) {
        use_screen();
    }
    for (int j = 0; j < h; j++) {
        int py = y + j;
        if (py < s_top || py >= s_bottom) {
            continue;
        }
        uint16_t *row = target_row(py);
        for (int i = 0; i < w; i++) {
            int px = x + i;
            if (px >= 0 && px < GFX_WIDTH) {
                row[px] = pixels[j * stride + i] & 0x7FFF;
            }
        }
    }
}

GFX_FAST void gfx_blit_half(int x, int y, int w, int h, const uint16_t *pixels, int stride)
{
    if (!s_base) {
        use_screen();
    }
    for (int j = 0; j < h; j++) {
        int py = y + j;
        if (py < s_top || py >= s_bottom) {
            continue;
        }
        const uint16_t *src = pixels + (2 * j) * stride;
        uint16_t *row = target_row(py);
        for (int i = 0; i < w; i++) {
            int px = x + i;
            if (px >= 0 && px < GFX_WIDTH) {
                row[px] = src[2 * i] & 0x7FFF;
            }
        }
    }
}

int gfx_text_width(const font_t *font, const char *text)
{
    glyph_t g;
    int width = 0;
    uint32_t c;
    while ((c = utf8_next(&text)) != 0) {
        font_glyph(font, c, &g);
        width += g.advance;
    }
    return width;
}

char *gfx_text_fit(const font_t *font, const char *text, int max_width, char *out, size_t out_size)
{
    size_t len = utf8_prefix(text, out_size - 1);
    memcpy(out, text, len);
    out[len] = '\0';

    /* One pass: remember the longest prefix that still fits with "...". */
    int limit = max_width - gfx_text_width(font, GFX_ELLIPSIS);
    int width = 0;
    size_t keep = 0;
    glyph_t g;
    const char *p = out;
    uint32_t c;
    while ((c = utf8_next(&p)) != 0) {
        font_glyph(font, c, &g);
        width += g.advance;
        if (width <= limit) {
            keep = (size_t)(p - out);
        }
    }
    if (width <= max_width) {
        return out;
    }
    while (keep > 0 && out[keep - 1] == ' ') {
        keep--;
    }
    if (keep + sizeof(GFX_ELLIPSIS) <= out_size) {
        memcpy(out + keep, GFX_ELLIPSIS, sizeof(GFX_ELLIPSIS));
    } else {
        out[keep] = '\0';
    }
    return out;
}

/* Glyphs are 1 bit per pixel: set pixels are drawn, the rest is left as is. */
GFX_FAST static void draw_glyph(const glyph_t *g, int x, int y, uint16_t color)
{
    int row_bytes = (g->width + 7) / 8;
    for (int j = 0; j < g->height; j++) {
        int py = y + g->y_offset + j;
        if (py < s_top || py >= s_bottom) {
            continue;
        }
        uint16_t *line = target_row(py);
        const uint8_t *row = g->bitmap + j * row_bytes;
        for (int i = 0; i < g->width; i++) {
            int px = x + g->x_offset + i;
            if ((row[i >> 3] & (0x80 >> (i & 7))) && px >= 0 && px < GFX_WIDTH) {
                line[px] = color;
            }
        }
    }
}

int gfx_text(const font_t *font, int x, int y, uint16_t color, const char *text, int max_width,
             gfx_align_t align)
{
    char fitted[160];
    if (max_width > 0) {
        text = gfx_text_fit(font, text, max_width, fitted, sizeof(fitted));
    }
    int width = gfx_text_width(font, text);
    if (align == ALIGN_CENTER) {
        x -= width / 2;
    } else if (align == ALIGN_RIGHT) {
        x -= width;
    }

    if (!s_base) {
        use_screen();
    }
    glyph_t g;
    uint32_t c;
    while ((c = utf8_next(&text)) != 0) {
        font_glyph(font, c, &g);
        if (g.width) {
            draw_glyph(&g, x, y, color);
        }
        x += g.advance;
    }
    return width;
}

void gfx_chevron(int x, int y, uint16_t color)
{
    /* Two-pixel-wide ">" shape, 7 pixels tall. */
    for (int i = 0; i < 4; i++) {
        gfx_fill(x + i, y + i, 2, 1, color);
        gfx_fill(x + i, y + 6 - i, 2, 1, color);
    }
}

void gfx_arrow(int x, int y, uint16_t color, bool up)
{
    for (int i = 0; i < 4; i++) {
        int row = up ? i : 3 - i;
        gfx_fill(x - i, y + row, 2 * i + 1, 1, color);
    }
}
