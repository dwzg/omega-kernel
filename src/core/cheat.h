/**
 * @file cheat.h
 * @brief Parser for EZ-FLASH `.cht` cheat files.
 *
 * A cheat file is an INI-like text file:
 *
 * @code
 * [Infinite Health]
 * ON=2001F2C,FF,03;2001F30,64
 * OFF=2001F2C,00
 *
 * [GameInfo]
 * Name=Castlevania - Aria of Sorrow
 * @endcode
 *
 * Each section is a cheat; each key inside it is one option, of which the
 * user can pick at most one. A value is a list of codes separated by ';'.
 * Each code is a hex address followed by one or more hex bytes separated by
 * ','; consecutive bytes go to consecutive addresses. A value may continue
 * on the following lines until a line containing '='.
 *
 * Parsing rules kept from the original kernel (existing cheat collections
 * rely on them):
 *  - spaces inside names are ignored;
 *  - a line starting with '#', '=' or '/', or shorter than two characters,
 *    ends the current section; lines inside a C-style block comment are
 *    skipped (the line that opens or closes the block is skipped too);
 *  - a line starting with "--" ends the list;
 *  - the `[GameInfo]` section and everything after it is not shown.
 *
 * Portable: reads through ::cht_reader_t, unit-tested on the host.
 */
#ifndef CORE_CHEAT_H
#define CORE_CHEAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Maximum length of a section or key name, including the NUL. */
#define CHT_NAME_LEN 50
/** Maximum length of a cheat value (all continuation lines together). */
#define CHT_VALUE_LEN 6000

/** Source of text lines, with fgets() semantics. */
typedef struct {
    void *ctx;
    /** Restart reading at the beginning of the file. */
    void (*rewind)(void *ctx);
    /**
     * Read up to @p size - 1 characters, stopping after a newline, and
     * NUL-terminate. Return false at end of file.
     */
    bool (*read_line)(void *ctx, char *buf, size_t size);
} cht_reader_t;

/** One line of the cheat menu: a section heading or an option. */
typedef struct {
    char name[CHT_NAME_LEN];  /**< Spaces removed: used to find the codes again. */
    char label[CHT_NAME_LEN]; /**< As written in the file: shown to the user. */
    uint8_t is_section;
    uint8_t selected;
} cht_entry_t;

/** Scratch memory needed while reading values (keep it out of IWRAM). */
typedef struct {
    char line[CHT_VALUE_LEN];
    char value[CHT_VALUE_LEN];
} cht_workspace_t;

/** A decoded cheat code: write @ref value to @ref address every frame. */
typedef struct {
    uint32_t address; /**< As written in the file (see patch.c for the mapping). */
    uint32_t value;
} cheat_code_t;

/**
 * @brief Build the cheat menu: every section and option, in file order,
 * stopping before `[GameInfo]`.
 * @return Number of entries written to @p entries.
 */
size_t cht_list_entries(const cht_reader_t *reader, cht_entry_t *entries, size_t max_entries);

/** @brief Read the game title from `[GameInfo]` `Name=` (empty if absent). */
void cht_read_game_name(const cht_reader_t *reader, char *out, size_t out_size);

/**
 * @brief Fetch the raw value of option @p key in section @p section.
 * The value is left in @p ws->value (not NUL-terminated). Spaces are
 * removed from the first line but kept in continuation lines.
 * @return Length of the value in bytes, 0 if not found.
 */
size_t cht_read_value(const cht_reader_t *reader, const char *section, const char *key,
                      cht_workspace_t *ws);

/**
 * @brief Decode a value ("ADDR,BB,BB;ADDR,BB") into cheat codes.
 * @param count In: codes already in @p codes. Out: new total (at most @p max).
 */
void cht_decode_value(const char *value, size_t len, cheat_code_t *codes, size_t max,
                      size_t *count);

/**
 * @brief Select option @p index and clear the other options of its section.
 * Selecting an already selected option clears it.
 */
void cht_toggle_option(cht_entry_t *entries, size_t count, size_t index);

/**
 * @brief Collect the codes of every selected option.
 * @return Number of codes written to @p codes.
 */
size_t cht_collect_codes(const cht_reader_t *reader, const cht_entry_t *entries, size_t count,
                         cht_workspace_t *ws, cheat_code_t *codes, size_t max_codes);

/** @brief Number of selected options. */
size_t cht_selected_count(const cht_entry_t *entries, size_t count);

/**
 * @brief Name of a cheat file in the shared cheat library.
 *
 * `/CHEAT/GameID2cht.bin` maps game codes to 4-character library numbers.
 * This builds the library file name ("1234.cht") and its folder
 * ("/CHEAT/Eng/1200"; folders hold 200 files each).
 */
void cht_library_paths(uint32_t library_id, char *folder, size_t folder_size, char *file,
                       size_t file_size);

#endif /* CORE_CHEAT_H */
