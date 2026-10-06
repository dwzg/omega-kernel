/**
 * @file font.c
 * @brief Bitmap font access. See font.h; the format is described in
 * tools/make_font.py.
 */
#include "gfx/font.h"

#define HEADER_SIZE 8
#define RANGE_SIZE 8
#define RECORD_SIZE 12

static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static unsigned read16(const uint8_t *p)
{
    return (unsigned)(p[0] | (p[1] << 8));
}

int font_line_height(const font_t *font)
{
    return font->data[4];
}

int font_ascent(const font_t *font)
{
    return font->data[5];
}

/** Record index of @p c, or -1 if the font has no such character. */
static int glyph_index(const uint8_t *d, uint32_t c)
{
    unsigned lo = 0;
    unsigned hi = read16(d + 6);
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        const uint8_t *r = d + HEADER_SIZE + mid * RANGE_SIZE;
        uint32_t first = read32(r);
        if (c < first) {
            hi = mid;
        } else if (c - first < read16(r + 4)) {
            return (int)(read16(r + 6) + (c - first));
        } else {
            lo = mid + 1;
        }
    }
    return -1;
}

void font_glyph(const font_t *font, uint32_t c, glyph_t *out)
{
    const font_t *f = font;
    int index = glyph_index(f->data, c);
    if (index < 0 && font->fallback) {
        f = font->fallback;
        index = glyph_index(f->data, c);
    }
    if (index < 0) {
        f = font;
        index = glyph_index(f->data, '?');
    }

    const uint8_t *d = f->data;
    unsigned ranges = read16(d + 6);
    const uint8_t *last = d + HEADER_SIZE + (ranges - 1) * RANGE_SIZE;
    unsigned total = read16(last + 6) + read16(last + 4);
    const uint8_t *records = d + HEADER_SIZE + ranges * RANGE_SIZE;
    const uint8_t *rec = records + index * RECORD_SIZE;
    const uint8_t *bitmaps = records + total * RECORD_SIZE;

    out->bitmap = bitmaps + read32(rec);
    out->width = rec[4];
    out->height = rec[5];
    out->x_offset = (int8_t)rec[6];
    out->y_offset = (int8_t)rec[7];
    out->advance = rec[8];
}
