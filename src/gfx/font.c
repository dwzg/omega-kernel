/**
 * @file font.c
 * @brief Bitmap font access. See font.h.
 */
#include "gfx/font.h"

#define HEADER_SIZE 8
#define RECORD_SIZE 8

int font_line_height(const font_t *font)
{
    return font->data[4];
}

int font_ascent(const font_t *font)
{
    return font->data[5];
}

void font_glyph(const font_t *font, unsigned char c, glyph_t *out)
{
    const uint8_t *d = font->data;
    unsigned first = d[6];
    unsigned count = d[7];

    if (c < first || c >= first + count) {
        c = '?';
    }
    const uint8_t *rec = d + HEADER_SIZE + (c - first) * RECORD_SIZE;
    const uint8_t *bitmaps = d + HEADER_SIZE + count * RECORD_SIZE;

    out->bitmap = bitmaps + (rec[0] | (rec[1] << 8));
    out->width = rec[2];
    out->height = rec[3];
    out->x_offset = (int8_t)rec[4];
    out->y_offset = (int8_t)rec[5];
    out->advance = rec[6];
}
