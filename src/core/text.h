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
 * @return true if the whole string fit, false if it was truncated.
 */
bool text_copy(char *dst, size_t size, const char *src);

/** @brief Remove trailing spaces, tabs, CR and LF in place. */
void text_trim_right(char *s);

/** @brief Case-insensitive test whether @p s ends with @p suffix. */
bool text_ends_with_ci(const char *s, const char *suffix);

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
