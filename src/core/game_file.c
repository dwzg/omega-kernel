/**
 * @file game_file.c
 * @brief Game file recognition. See game_file.h.
 */
#include "core/game_file.h"

#include <stdio.h>
#include <string.h>

#include "core/text.h"

bool game_file_is_gba(const char *filename)
{
    return text_ends_with_ci(filename, ".gba");
}

bool game_file_companion(char *dst, size_t size, const char *filename, const char *ext3)
{
    size_t len = strlen(filename);
    if (len < 3) {
        return false;
    }
    int n = snprintf(dst, size, "%.*s%s", (int)(len - 3), filename, ext3);
    return n > 0 && (size_t)n < size;
}
