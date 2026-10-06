/**
 * @file save_type.h
 * @brief Save-memory types, their FPGA codes and save file sizes.
 *
 * The OMEGA FPGA emulates a game's save chip. It is told which chip to
 * emulate with a "save mode" byte. The user can let the kernel pick the type
 * from a database (::SAVE_CHOICE_AUTO) or force one in the game menu; the
 * choice is remembered per game in a `.mde` file.
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_SAVE_TYPE_H
#define CORE_SAVE_TYPE_H

#include <stdbool.h>
#include <stdint.h>

/** FPGA save-mode codes. */
typedef enum {
    SAVE_MODE_NONE = 0x00,          /**< Game has no save memory. */
    SAVE_MODE_SRAM_64K = 0x10,      /**< Unknown games and homebrew: 64 KiB SRAM. */
    SAVE_MODE_SRAM = 0x11,          /**< 32 KiB SRAM. */
    SAVE_MODE_EEPROM_512 = 0x21,    /**< 512 B EEPROM. */
    SAVE_MODE_EEPROM_8K = 0x22,     /**< 8 KiB EEPROM. */
    SAVE_MODE_EEPROM_8K_BIG = 0x23, /**< 8 KiB EEPROM in a ROM larger than 16 MiB. */
    SAVE_MODE_FLASH_128K = 0x31,    /**< 128 KiB flash (two banks). */
    SAVE_MODE_FLASH_64K = 0x32,     /**< 64 KiB flash. */
    SAVE_MODE_FLASH512_64K = 0x33   /**< 64 KiB flash (512 Kbit variant). */
} save_mode_t;

/** What the user selected in the "Save type" menu row (stored in `.mde`). */
typedef enum {
    SAVE_CHOICE_AUTO = 0,       /**< Look the game up in the database. */
    SAVE_CHOICE_SRAM = 1,       /**< Force 32 KiB SRAM. */
    SAVE_CHOICE_EEPROM_8K = 2,  /**< Force 8 KiB EEPROM. */
    SAVE_CHOICE_EEPROM_512 = 3, /**< Force 512 B EEPROM. */
    SAVE_CHOICE_FLASH_64K = 4,  /**< Force 64 KiB flash. */
    SAVE_CHOICE_FLASH_128K = 5, /**< Force 128 KiB flash. */
    SAVE_CHOICE_COUNT = 6
} save_choice_t;

/** One record of the save type database (see src/data/save_type_db.c). */
typedef struct {
    char game_code[4]; /**< Game code from the ROM header, not NUL-terminated. */
    uint8_t save_mode; /**< ::save_mode_t */
} save_type_db_entry_t;

/** The database; terminated by an entry whose code is "FFFF". */
extern const save_type_db_entry_t save_type_db[];

/** Save mode used when a game is not in the database. */
#define SAVE_MODE_DEFAULT SAVE_MODE_SRAM_64K

/** @brief Whether @p game_code is in the database. */
bool save_type_known(const char game_code[4]);

/**
 * @brief Look a game code up in the database.
 * @return The game's save mode, or ::SAVE_MODE_DEFAULT if unknown.
 */
save_mode_t save_type_lookup(const char game_code[4]);

/**
 * @brief Turn the user's choice into the save mode to program.
 * @param choice     The menu selection.
 * @param game_code  Used for ::SAVE_CHOICE_AUTO.
 * @param rom_size   ROM size in bytes; EEPROM games larger than 18 MiB need
 *                   ::SAVE_MODE_EEPROM_8K_BIG.
 */
save_mode_t save_type_resolve(save_choice_t choice, const char game_code[4], uint32_t rom_size);

/**
 * Bytes consecutive blocks passed to save_type_detect() must overlap, so a
 * marker that straddles two blocks is still found.
 */
#define SAVE_TYPE_MARKER_OVERLAP 16

/**
 * @brief Find the save type of a game from its code.
 *
 * Games built with Nintendo's save library contain its version string,
 * which names the chip: "EEPROM_V", "SRAM_V", "SRAM_F_V", "FLASH_V",
 * "FLASH512_V" or "FLASH1M_V". The strings are word-aligned, so only word
 * starts are checked.
 *
 * @param words    A block of the ROM.
 * @param count    Number of words in the block.
 * @param rom_size ROM size in bytes (EEPROM games over 18 MiB need
 *                 ::SAVE_MODE_EEPROM_8K_BIG).
 * @return The save mode, or ::SAVE_MODE_DEFAULT if the block names none.
 *         EEPROM is reported as 8K, by far the most common size.
 */
save_mode_t save_type_detect(const uint32_t *words, uint32_t count, uint32_t rom_size);

/** @brief Size of a newly created save file for @p mode, in bytes (0 = none). */
uint32_t save_type_file_size(save_mode_t mode);

/** @brief Short label for a menu choice, e.g. "Auto", "SRAM", "Flash 128K". */
const char *save_choice_name(save_choice_t choice);

/** @brief Short label for a save mode, e.g. "SRAM 32K", "EEPROM 8K", "None". */
const char *save_mode_name(save_mode_t mode);

#endif /* CORE_SAVE_TYPE_H */
