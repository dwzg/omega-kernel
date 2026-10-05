/**
 * @file stubs.c
 * @brief Replacements for the hardware-only loader modules.
 */
#include <stdio.h>
#include <string.h>

#include "loader/boot.h"
#include "loader/firmware.h"
#include "loader/nor_games.h"
#include "sim.h"

nor_entry_t g_nor_table[NOR_MAX_GAMES];
static unsigned s_nor_count;
static unsigned s_boot_requests;

/* --- Starting games: never possible in the simulator. -------------------- */

boot_result_t boot_sd_game(const boot_request_t *req, const settings_t *settings,
                           const progress_t *progress)
{
    s_boot_requests++;
    progress_status(progress, "Loading...");
    progress_advance(progress, 1, 2);
    return BOOT_ERR_GAME_FILE;
}

boot_result_t boot_nor_game(unsigned index, bool bios_boot, const settings_t *settings,
                            const progress_t *progress)
{
    s_boot_requests++;
    return BOOT_ERR_NOR_MISSING;
}

unsigned sim_boot_requests(void)
{
    return s_boot_requests;
}

/* --- NOR library: an in-memory table. ------------------------------------ */

void sim_nor_set_games(unsigned count)
{
    static const struct {
        const char *file;
        const char *header;
        uint32_t size;
    } GAMES[] = {
        {"Metroid Fusion.gba", "METROID4USA AMTE", 8u << 20},
        {"Golden Sun.gba", "Golden_Sun_AAGSE", 8u << 20},
        {"Advance Wars.gba", "ADVANCEWARS AWRE", 4u << 20},
        {"Mother 3.gba", "MOTHER3     A3UJ", 32u << 20},
    };
    memset(g_nor_table, 0, sizeof(g_nor_table));
    s_nor_count = count < 4 ? count : 4;
    for (unsigned i = 0; i < s_nor_count; i++) {
        snprintf(g_nor_table[i].filename, sizeof(g_nor_table[i].filename), "%s", GAMES[i].file);
        memcpy(g_nor_table[i].header_title, GAMES[i].header, 16);
        g_nor_table[i].size = GAMES[i].size;
        g_nor_table[i].has_hooks = i == 1;
    }
}

void nor_games_init(void)
{
}

void nor_games_load_table(void)
{
}

void nor_games_save_table(void)
{
}

unsigned nor_games_scan(uint32_t *used_bytes)
{
    *used_bytes = 0;
    for (unsigned i = 0; i < s_nor_count; i++) {
        *used_bytes += g_nor_table[i].size;
    }
    return s_nor_count;
}

void nor_game_delete_last(unsigned count, uint32_t used_bytes)
{
    if (s_nor_count > 0) {
        s_nor_count--;
    }
}

void nor_games_erase_all(void (*poll)(uint32_t tick))
{
    for (uint32_t t = 0; t < 120; t++) {
        poll(t);
    }
    s_nor_count = 0;
}

/* --- Firmware: always current. -------------------------------------------- */

bool firmware_update_available(uint16_t version)
{
    return false;
}

bool firmware_image_valid(void)
{
    return true;
}

void firmware_write(const progress_t *progress)
{
}
