/**
 * @file save_type.c
 * @brief Save type mapping and database lookup. See save_type.h.
 */
#include "core/save_type.h"

#include <string.h>

/** ROM size above which a forced 8K EEPROM uses ::SAVE_MODE_EEPROM_8K_BIG. */
#define EEPROM_BIG_ROM_THRESHOLD 0x1200000u

save_mode_t save_type_lookup(const char game_code[4])
{
    for (const save_type_db_entry_t *e = save_type_db; memcmp(e->game_code, "FFFF", 4) != 0; e++) {
        if (memcmp(e->game_code, game_code, 4) == 0) {
            return (save_mode_t)e->save_mode;
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
