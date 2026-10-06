/**
 * @file text.c
 * @brief String helpers. See text.h.
 */
#include "core/text.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "core/utf8.h"

bool text_copy(char *dst, size_t size, const char *src)
{
    if (size == 0) {
        return false;
    }
    size_t len = strlen(src);
    bool fits = len < size;
    if (!fits) {
        len = utf8_prefix(src, size - 1); /* never cut a character in half */
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
    return fits;
}

void text_trim_right(char *s)
{
    size_t n = strlen(s);
    while (n > 0) {
        char c = s[n - 1];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            break;
        }
        s[--n] = '\0';
    }
}

bool text_ends_with_ci(const char *s, const char *suffix)
{
    size_t n = strlen(s);
    size_t m = strlen(suffix);
    return n >= m && strcasecmp(s + n - m, suffix) == 0;
}

uint32_t text_parse_hex(const char *s)
{
    if (strlen(s) > 8) {
        return 0;
    }
    uint32_t value = 0;
    for (; *s; s++) {
        char c = *s;
        if (c >= '0' && c <= '9') {
            value = value * 16 + (uint32_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            value = value * 16 + (uint32_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            value = value * 16 + (uint32_t)(c - 'A' + 10);
        }
    }
    return value;
}

char *text_format_size(char *dst, size_t size, uint32_t bytes)
{
    if (bytes >= 1024u * 1024u) {
        snprintf(dst, size, "%lu MB", (unsigned long)(bytes >> 20));
    } else if (bytes >= 1024u) {
        snprintf(dst, size, "%lu KB", (unsigned long)(bytes >> 10));
    } else {
        snprintf(dst, size, "%lu B", (unsigned long)bytes);
    }
    return dst;
}

/* Base letters of U+00C0..U+017F; 0 = not a letter (x, /). Generated from
 * the Unicode decompositions. */
static const char FOLD_LATIN[] = "aaaaaaaceeeeiiiidnooooo\0ouuuuyts" /* U+00C0 */
                                 "aaaaaaaceeeeiiiidnooooo\0ouuuuyty" /* U+00E0 */
                                 "aaaaaaccccccccddddeeeeeeeeeegggg"  /* U+0100 */
                                 "gggghhhhiiiiiiiiiiiijjkkklllllll"  /* U+0120 */
                                 "lllnnnnnnnnnoooooooorrrrrrssssss"  /* U+0140 */
                                 "ssttttttuuuuuuuuuuuuwwyyyzzzzzzs"; /* U+0160 */

uint32_t text_fold(uint32_t c)
{
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    if (c >= 0xC0 && c < 0x180 && FOLD_LATIN[c - 0xC0]) {
        return (uint32_t)FOLD_LATIN[c - 0xC0];
    }
    return c;
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

/** Compare the digit runs at *a and *b by value, and step over them. */
static int compare_numbers(const char **a, const char **b)
{
    while (**a == '0') {
        (*a)++;
    }
    while (**b == '0') {
        (*b)++;
    }
    const char *da = *a;
    const char *db = *b;
    while (is_digit(**a)) {
        (*a)++;
    }
    while (is_digit(**b)) {
        (*b)++;
    }
    size_t la = (size_t)(*a - da);
    size_t lb = (size_t)(*b - db);
    if (la != lb) {
        return la < lb ? -1 : 1; /* more digits: a larger number */
    }
    for (size_t i = 0; i < la; i++) {
        if (da[i] != db[i]) {
            return da[i] < db[i] ? -1 : 1;
        }
    }
    return 0;
}

int text_compare_names(const char *a, const char *b)
{
    const char *pa = a;
    const char *pb = b;
    for (;;) {
        if (is_digit(*pa) && is_digit(*pb)) {
            int r = compare_numbers(&pa, &pb);
            if (r) {
                return r;
            }
            continue;
        }
        uint32_t ca;
        uint32_t cb;
        if ((uint8_t)*pa < 0x80 && (uint8_t)*pb < 0x80) {
            /* Fast path for plain ASCII, by far the most common. */
            ca = (uint8_t)*pa;
            cb = (uint8_t)*pb;
            ca += (ca >= 'A' && ca <= 'Z') ? 'a' - 'A' : 0;
            cb += (cb >= 'A' && cb <= 'Z') ? 'a' - 'A' : 0;
            pa += ca != 0;
            pb += cb != 0;
        } else {
            ca = text_fold(utf8_next(&pa));
            cb = text_fold(utf8_next(&pb));
        }
        if (ca != cb) {
            return ca < cb ? -1 : 1;
        }
        if (ca == 0) {
            return strcmp(a, b);
        }
    }
}
