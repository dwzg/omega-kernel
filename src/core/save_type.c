/**
 * @file save_type.c
 * @brief Save type mapping and database lookup. See save_type.h.
 */
#include "core/save_type.h"

#include <string.h>

#include "platform/attributes.h"

/** ROM size above which a forced 8K EEPROM uses ::SAVE_MODE_EEPROM_8K_BIG. */
#define EEPROM_BIG_ROM_THRESHOLD 0x1200000u

static const save_type_db_entry_t *find(const char game_code[4])
{
    for (const save_type_db_entry_t *e = save_type_db; memcmp(e->game_code, "FFFF", 4) != 0; e++) {
        if (memcmp(e->game_code, game_code, 4) == 0) {
            return e;
        }
    }
    return NULL;
}

bool save_type_known(const char game_code[4])
{
    return find(game_code) != NULL;
}

save_mode_t save_type_lookup(const char game_code[4])
{
    const save_type_db_entry_t *e = find(game_code);
    return e ? (save_mode_t)e->save_mode : SAVE_MODE_DEFAULT;
}

/** @p s (NUL-terminated) is at @p p. No library calls: runs from IWRAM. */
static bool PLATFORM_FAST_CODE starts_with(const uint8_t *p, const char *s)
{
    for (; *s; p++, s++) {
        if (*p != (uint8_t)*s) {
            return false;
        }
    }
    return true;
}

/* Little-endian words of the marker beginnings. */
#define WORD_EEPR 0x52504545u /* "EEPR" */
#define WORD_SRAM 0x4D415253u /* "SRAM" */
#define WORD_FLAS 0x53414C46u /* "FLAS" */

save_mode_t PLATFORM_FAST_CODE save_type_detect(const uint32_t *words, uint32_t count,
                                                uint32_t rom_size)
{
    /* The longest marker, "FLASH512_V", spans three words. */
    for (uint32_t i = 0; i + 3 <= count; i++) {
        uint32_t w = words[i];
        if (w != WORD_EEPR && w != WORD_SRAM && w != WORD_FLAS) {
            continue;
        }
        const uint8_t *p = (const uint8_t *)&words[i];
        if (starts_with(p, "EEPROM_V")) {
            return rom_size > EEPROM_BIG_ROM_THRESHOLD ? SAVE_MODE_EEPROM_8K_BIG
                                                       : SAVE_MODE_EEPROM_8K;
        }
        if (starts_with(p, "SRAM_V") || starts_with(p, "SRAM_F_V")) {
            return SAVE_MODE_SRAM;
        }
        if (starts_with(p, "FLASH_V")) {
            return SAVE_MODE_FLASH_64K;
        }
        if (starts_with(p, "FLASH512_V")) {
            return SAVE_MODE_FLASH512_64K;
        }
        if (starts_with(p, "FLASH1M_V")) {
            return SAVE_MODE_FLASH_128K;
        }
    }
    return SAVE_MODE_DEFAULT;
}

save_mode_t save_type_resolve(save_choice_t choice, const char game_code[4], uint32_t rom_size)
{
    switch (choice) {
    case SAVE_CHOICE_AUTO:
        return save_type_lookup(game_code);
    case SAVE_CHOICE_SRAM:
        return SAVE_MODE_SRAM;
    case SAVE_CHOICE_EEPROM_8K:
        return rom_size > EEPROM_BIG_ROM_THRESHOLD ? SAVE_MODE_EEPROM_8K_BIG : SAVE_MODE_EEPROM_8K;
    case SAVE_CHOICE_EEPROM_512:
        return SAVE_MODE_EEPROM_512;
    case SAVE_CHOICE_FLASH_64K:
        return SAVE_MODE_FLASH_64K;
    case SAVE_CHOICE_FLASH_128K:
        return SAVE_MODE_FLASH_128K;
    default:
        return SAVE_MODE_NONE;
    }
}

uint32_t save_type_file_size(save_mode_t mode)
{
    switch (mode) {
    case SAVE_MODE_NONE:
        return 0;
    case SAVE_MODE_SRAM:
        return 0x8000;
    case SAVE_MODE_EEPROM_512:
        return 0x200;
    case SAVE_MODE_EEPROM_8K:
    case SAVE_MODE_EEPROM_8K_BIG:
        return 0x2000;
    case SAVE_MODE_FLASH_64K:
    case SAVE_MODE_FLASH512_64K:
        return 0x10000;
    case SAVE_MODE_FLASH_128K:
        return 0x20000;
    case SAVE_MODE_SRAM_64K:
    default:
        /* Unknown games (often homebrew) get 64 KiB of SRAM. */
        return 0x10000;
    }
}

const char *save_choice_name(save_choice_t choice)
{
    static const char *const names[SAVE_CHOICE_COUNT] = {
        "Auto", "SRAM", "EEPROM 8K", "EEPROM 512", "Flash 64K", "Flash 128K",
    };
    return (unsigned)choice < SAVE_CHOICE_COUNT ? names[choice] : "Auto";
}

const char *save_mode_name(save_mode_t mode)
{
    switch (mode) {
    case SAVE_MODE_NONE:
        return "None";
    case SAVE_MODE_SRAM:
        return "SRAM 32K";
    case SAVE_MODE_EEPROM_512:
        return "EEPROM 512";
    case SAVE_MODE_EEPROM_8K:
    case SAVE_MODE_EEPROM_8K_BIG:
        return "EEPROM 8K";
    case SAVE_MODE_FLASH_64K:
    case SAVE_MODE_FLASH512_64K:
        return "Flash 64K";
    case SAVE_MODE_FLASH_128K:
        return "Flash 128K";
    case SAVE_MODE_SRAM_64K:
    default:
        return "SRAM 64K";
    }
}
