/**
 * @file utf8.c
 * @brief UTF-8 decoding. See utf8.h.
 */
#include "core/utf8.h"

uint32_t utf8_next(const char **s)
{
    const uint8_t *p = (const uint8_t *)*s;
    uint32_t c = p[0];
    unsigned extra;
    uint32_t min;

    if (c == 0) {
        return 0;
    }
    if (c < 0x80) {
        *s += 1;
        return c;
    }
    if ((c & 0xE0) == 0xC0) {
        extra = 1;
        min = 0x80;
        c &= 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        extra = 2;
        min = 0x800;
        c &= 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        extra = 3;
        min = 0x10000;
        c &= 0x07;
    } else {
        *s += 1; /* stray continuation byte or invalid lead byte */
        return UTF8_INVALID;
    }
    for (unsigned i = 1; i <= extra; i++) {
        if ((p[i] & 0xC0) != 0x80) { /* also stops at the terminating 0 */
            *s += 1;
            return UTF8_INVALID;
        }
        c = (c << 6) | (p[i] & 0x3F);
    }
    *s += 1 + extra;
    /* Overlong forms, surrogates and values beyond Unicode are invalid. */
    if (c < min || (c >= 0xD800 && c <= 0xDFFF) || c > 0x10FFFF) {
        return UTF8_INVALID;
    }
    return c;
}

size_t utf8_prefix(const char *s, size_t max)
{
    size_t n = 0;
    while (n < max && s[n]) {
        const char *p = s + n;
        utf8_next(&p);
        size_t step = (size_t)(p - (s + n));
        if (n + step > max) {
            break;
        }
        n += step;
    }
    return n;
}
