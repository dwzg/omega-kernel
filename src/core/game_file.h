/**
 * @file game_file.h
 * @brief Game file recognition and the names of companion files.
 *
 * The kernel runs GBA games only (`*.gba`). For a game "Name.gba" it keeps
 * companion files in fixed folders on the SD card (see docs/sd-card-layout.md):
 *
 * | File            | Folder    | Purpose                               |
 * |-----------------|-----------|---------------------------------------|
 * | `Name.sav`      | `/SAVER`  | Save data                             |
 * | `Name.mde`      | `/SAVER`  | Save type chosen by the user          |
 * | `Name.rts`      | `/RTS`    | Real-time save state                  |
 * | `Name.pat`      | `/PATCH`  | Cached patch locations                |
 * | `Name.cht`      | `/CHEAT`  | Cheat codes                           |
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_GAME_FILE_H
#define CORE_GAME_FILE_H

#include <stdbool.h>
#include <stddef.h>

/** @brief true if @p filename ends in ".gba" (case-insensitive). */
bool game_file_is_gba(const char *filename);

/**
 * @brief Derive a companion file name by replacing the 3-character extension.
 *
 * "Name.gba" + "sav" -> "Name.sav".
 * @return false if @p filename is shorter than 3 characters or the result
 *         does not fit into @p size bytes.
 */
bool game_file_companion(char *dst, size_t size, const char *filename, const char *ext3);

#endif /* CORE_GAME_FILE_H */
