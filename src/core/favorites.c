/**
 * @file favorites.c
 * @brief The list of favorite games. See favorites.h.
 */
#include "core/favorites.h"

#include <string.h>

#include "core/text.h"

void favorites_clear(favorites_t *list)
{
    list->count = 0;
}

bool favorites_append_line(favorites_t *list, const char *line)
{
    char entry[RECENT_ENTRY_LEN];

    if (list->count >= FAVORITES_MAX) {
        return false;
    }
    if (!text_copy(entry, sizeof(entry), line)) {
        return true; /* too long to be ours: skip */
    }
    text_trim_right(entry);
    if (entry[0] == '/' && favorites_find(list, entry) < 0) {
        memcpy(list->entries[list->count++], entry, sizeof(entry));
    }
    return true;
}

int favorites_find(const favorites_t *list, const char *path)
{
    for (unsigned i = 0; i < list->count; i++) {
        if (strcmp(list->entries[i], path) == 0) {
            return (int)i;
        }
    }
    return -1;
}

bool favorites_add(favorites_t *list, const char *path)
{
    if (favorites_find(list, path) >= 0) {
        return true;
    }
    if (list->count >= FAVORITES_MAX || strlen(path) >= RECENT_ENTRY_LEN) {
        return false;
    }
    text_copy(list->entries[list->count++], RECENT_ENTRY_LEN, path);
    return true;
}

void favorites_remove(favorites_t *list, const char *path)
{
    int found = favorites_find(list, path);
    if (found < 0) {
        return;
    }
    list->count--;
    memmove(list->entries[found], list->entries[found + 1],
            (list->count - (unsigned)found) * sizeof(list->entries[0]));
}
