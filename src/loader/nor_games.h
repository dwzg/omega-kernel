/**
 * @file nor_games.h
 * @brief Games stored in the 64 MiB NOR flash.
 *
 * Games are written back to back from offset 0, each padded to a whole
 * 128 KiB block. A table in the S71 flash (::CONFIG_NOR_TABLE_OFFSET)
 * records name, size and options of each game. The list of games is
 * rebuilt at start-up by walking the NOR flash and checking each ROM header
 * against the table (nor_games_scan()), so a game whose first block was
 * erased disappears from the list.
 *
 * Only the most recently written game can be deleted.
 */
#ifndef LOADER_NOR_GAMES_H
#define LOADER_NOR_GAMES_H

#include <stdbool.h>
#include <stdint.h>

#include "loader/progress.h"
#include "patch/patch.h"

/** Size of the NOR flash in bytes (64 MiB). */
#define NOR_GAMES_CAPACITY 0x04000000u

/** Maximum number of games in NOR. */
#define NOR_MAX_GAMES 64

/**
 * One table entry. This exact layout (132 bytes) is stored in flash and
 * shared with earlier kernels - do not change it.
 */
typedef struct {
    char filename[100];      /**< File name the game was written from. */
    uint16_t rom_page;       /**< ROM page register value that maps the game. */
    uint16_t has_hooks;      /**< Written with in-game hooks. */
    uint16_t has_save_state; /**< The save-state hook was enabled. */
    uint16_t reserved;
    uint32_t size; /**< Bytes occupied in NOR (multiple of 128 KiB). */
    uint32_t reserved2;
    char header_title[16]; /**< ROM header bytes 0xA0-0xAF (title + game code). */
} nor_entry_t;

_Static_assert(sizeof(nor_entry_t) == 132, "NOR table layout must not change");

/** The table, as loaded from flash. */
extern nor_entry_t g_nor_table[NOR_MAX_GAMES];

/** Result of nor_game_write(). */
typedef enum {
    NOR_WRITE_OK,
    NOR_WRITE_NO_CHIP, /**< NOR flash not detected. */
    NOR_WRITE_OPEN_FAILED,
    NOR_WRITE_FULL, /**< Not enough space or no free table entry. */
} nor_write_result_t;

/** @brief Load the table from flash. */
void nor_games_load_table(void);

/**
 * @brief Load the table at start-up and drop stale entries.
 *
 * If no game is found in NOR, a leftover table (e.g. after the chip was
 * erased elsewhere) is cleared so that new games start at entry 0. The
 * flash is only written when the table is not already empty.
 */
void nor_games_init(void);

/** @brief Store the table in flash. */
void nor_games_save_table(void);

/**
 * @brief Count the games present in NOR.
 * @param used_bytes Receives the offset where the next game would go.
 * @return Number of games (they are entries 0 .. n-1 of ::g_nor_table).
 */
unsigned nor_games_scan(uint32_t *used_bytes);

/**
 * @brief Write a game to NOR after the existing ones.
 * @param patch      Initialised patch context for the game. With hooks,
 *                   the trim size must already be determined.
 * @param with_hooks Install the in-game hooks (otherwise only game fixes).
 */
nor_write_result_t nor_game_write(const char *path, unsigned index, uint32_t offset,
                                  patch_context_t *patch, bool with_hooks,
                                  const progress_t *progress);

/** @brief Delete the last game (erases its first block). */
void nor_game_delete_last(unsigned count, uint32_t used_bytes);

/**
 * @brief Erase the whole NOR flash and clear the table (takes minutes).
 * @param poll Called while waiting, see nor_erase_chip().
 */
void nor_games_erase_all(void (*poll)(uint32_t tick));

#endif /* LOADER_NOR_GAMES_H */
