/**
 * @file game_info.h
 * @brief Facts about a game file read from the SD card.
 */
#ifndef LOADER_GAME_INFO_H
#define LOADER_GAME_INFO_H

#include <stdbool.h>
#include <stdint.h>

/** Offset of the 4-character game code in a GBA ROM header. */
#define ROM_HEADER_GAME_CODE 0xAC
/** Offset of the 12-character title in a GBA ROM header. */
#define ROM_HEADER_TITLE 0xA0
/** Largest ROM the cartridge can run (32 MiB). */
#define ROM_MAX_SIZE 0x02000000u

typedef struct {
    uint32_t size;     /**< File size in bytes. */
    char game_code[5]; /**< NUL-terminated; "FFFF" if unknown. */
} game_info_t;

/**
 * @brief Read size and game code of @p path.
 * @return false if the file cannot be opened (info is still filled with defaults).
 */
bool game_info_read(const char *path, game_info_t *info);

#endif /* LOADER_GAME_INFO_H */
