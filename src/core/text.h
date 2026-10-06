/**
 * @file text.h
 * @brief Small, bounds-checked string helpers.
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_TEXT_H
#define CORE_TEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Copy @p src into @p dst (capacity @p size), always NUL-terminating.
 * A truncated copy never ends inside a UTF-8 character.
 * @return true if the whole string fit, false if it was truncated.
 */
bool text_copy(char *dst, size_t size, const char *src);

/** @brief Remove trailing spaces, tabs, CR and LF in place. */
void text_trim_right(char *s);

/** @brief Case-insensitive test whether @p s ends with @p suffix. */
bool text_ends_with_ci(const char *s, const char *suffix);

/**
 * @brief The character @p c is sorted as: letters in lower case, accented
 * Latin letters as their base letter ('É' -> 'e', 'ß' -> 's').
 */
uint32_t text_fold(uint32_t c);

/**
 * @brief Compare two UTF-8 names the way people expect a list to be sorted.
 *
 * Case and accents are ignored (see text_fold()), and runs of digits are
 * compared by value, so "Game 2" comes before "Game 10". Names that are equal
 * that way are ordered byte by byte, so the order is always the same.
 * @return <0, 0 or >0 like strcmp().
 */
int text_compare_names(const char *a, const char *b);

/**
 * @brief Parse up to 8 hexadecimal digits.
 *
 * Characters that are not hex digits are skipped. Strings longer than 8
 * characters yield 0 (the format used by cheat files never needs more).
 */
uint32_t text_parse_hex(const char *s);

/**
 * @brief Format a byte count for display: "8 MB", "384 KB" or "512 B".
 * @return @p dst.
 */
char *text_format_size(char *dst, size_t size, uint32_t bytes);

#endif /* CORE_TEXT_H */
