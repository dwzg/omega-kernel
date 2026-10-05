/**
 * @file path.h
 * @brief Manipulation of absolute, '/'-separated FatFs paths.
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_PATH_H
#define CORE_PATH_H

#include <stdbool.h>
#include <stddef.h>

/** Maximum length of a path including the terminating NUL. */
#define PATH_MAX_LEN 256

/**
 * @brief Join a directory and an entry name: ("/", "a") -> "/a", ("/x", "a") -> "/x/a".
 * @return false (and leaves @p dst unspecified) if the result does not fit.
 */
bool path_join(char *dst, size_t size, const char *dir, const char *name);

/**
 * @brief Remove the last component in place: "/x/y" -> "/x", "/x" -> "/".
 * The root "/" is left unchanged.
 */
void path_to_parent(char *path);

/** @brief Return a pointer to the last component of @p path (after the last '/'). */
const char *path_basename(const char *path);

/**
 * @brief Split "/dir/file" into its directory and file name.
 * @return false if @p path has no '/', or a part does not fit.
 */
bool path_split(const char *path, char *dir, size_t dir_size, char *name, size_t name_size);

#endif /* CORE_PATH_H */
