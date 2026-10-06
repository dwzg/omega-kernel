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
