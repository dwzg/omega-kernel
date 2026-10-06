/**
 * @file utf8.h
 * @brief Decoding UTF-8 text (file names are UTF-8, see docs/sd-card-layout.md).
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_UTF8_H
#define CORE_UTF8_H

#include <stddef.h>
#include <stdint.h>

/** Code point returned for bytes that are not valid UTF-8. */
#define UTF8_INVALID 0xFFFDu

/**
 * @brief Decode the character at @p *s and advance @p *s past it.
 *
 * Invalid or truncated sequences decode to ::UTF8_INVALID and advance by
 * one byte, so any byte string can be walked safely. Returns 0 at the end
 * of the string (without advancing).
 */
uint32_t utf8_next(const char **s);

/**
 * @brief Length of the longest prefix of @p s (at most @p max bytes) that
 * does not end inside a multi-byte character.
 */
size_t utf8_prefix(const char *s, size_t max);

#endif /* CORE_UTF8_H */
