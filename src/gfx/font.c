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
    unsigned ranges = read16(d + 6);
    unsigned base = 0;
    for (unsigned i = 0; i < ranges; i++) {
        const uint8_t *r = d + HEADER_SIZE + i * RANGE_SIZE;
        uint32_t first = read32(r);
        unsigned count = read16(r + 4);
        if (c >= first && c - first < count) {
            return (int)(base + (c - first));
        }
        base += count;
    }
    return -1;
}

void font_glyph(const font_t *font, uint32_t c, glyph_t *out)
{
    const uint8_t *d = font->data;
    unsigned ranges = read16(d + 6);
    unsigned total = 0;
    for (unsigned i = 0; i < ranges; i++) {
        total += read16(d + HEADER_SIZE + i * RANGE_SIZE + 4);
    }

    int index = glyph_index(d, c);
    if (index < 0) {
        index = glyph_index(d, '?');
    }
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
