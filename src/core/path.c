/**
 * @file path.c
 * @brief Path helpers. See path.h.
 */
#include "core/path.h"

#include <stdio.h>
#include <string.h>

#include "core/text.h"

bool path_join(char *dst, size_t size, const char *dir, const char *name)
{
    bool at_root = strcmp(dir, "/") == 0;
    int len = snprintf(dst, size, "%s%s%s", dir, at_root ? "" : "/", name);
    return len > 0 && (size_t)len < size;
}

void path_to_parent(char *path)
{
    char *slash = strrchr(path, '/');
    if (!slash) {
        return;
    }
    if (slash == path) {
        path[1] = '\0';
    } else {
        *slash = '\0';
    }
}

const char *path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

bool path_split(const char *path, char *dir, size_t dir_size, char *name, size_t name_size)
{
    const char *slash = strrchr(path, '/');
    if (!slash) {
        return false;
    }
    size_t dir_len = (size_t)(slash - path);
    if (dir_len == 0) {
        dir_len = 1; /* keep the root "/" */
    }
    if (dir_len >= dir_size) {
        return false;
    }
    memcpy(dir, path, dir_len);
    dir[dir_len] = '\0';
    return text_copy(name, name_size, slash + 1);
}
