/**
 * @file recent.c
 * @brief Most-recently-played list. See recent.h.
 */
#include "core/recent.h"

#include <string.h>

#include "core/text.h"

void recent_clear(recent_list_t *list)
{
    list->count = 0;
}

bool recent_append_line(recent_list_t *list, const char *line)
{
    char entry[RECENT_ENTRY_LEN];

    if (list->count >= RECENT_MAX) {
        return false;
    }
    text_copy(entry, sizeof(entry), line);
    text_trim_right(entry);
    if (entry[0] != '/') {
        return false;
    }
    memcpy(list->entries[list->count++], entry, sizeof(entry));
    return true;
}

void recent_touch(recent_list_t *list, const char *path)
{
    unsigned found;

    for (found = 0; found < list->count; found++) {
        if (strcmp(list->entries[found], path) == 0) {
            break;
        }
    }
    if (found == list->count) {
        /* New entry: grow the list, or reuse the slot of the oldest entry. */
        if (list->count < RECENT_MAX) {
            list->count++;
        }
        found = list->count - 1;
    }
    for (unsigned i = found; i > 0; i--) {
        memcpy(list->entries[i], list->entries[i - 1], RECENT_ENTRY_LEN);
    }
    text_copy(list->entries[0], RECENT_ENTRY_LEN, path);
}
