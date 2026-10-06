/**
 * @file nor_games.c
 * @brief NOR game library. See nor_games.h.
 */
#include "loader/nor_games.h"

#include <gba_interrupt.h>
#include <stdio.h>
#include <string.h>

#include "core/path.h"
#include "core/text.h"
#include "ff.h"
#include "hal/config_flash.h"
#include "hal/nor.h"
#include "hal/omega.h"
#include "hal/platform.h"
#include "loader/buffers.h"
#include "loader/game_info.h"
#include "loader/sd_paths.h"
#include "platform/attributes.h"

nor_entry_t g_nor_table[NOR_MAX_GAMES] PLATFORM_EWRAM;

_Static_assert(NOR_GAMES_CAPACITY == NOR_TOTAL_SIZE, "NOR size mismatch");

/** ROM header bytes used to recognise a written game. */
#define HEADER_FIXED_OFFSET 0xB2 /* always 0x96 */
#define HEADER_FIXED_VALUE 0x96
#define HEADER_RESERVED_OFFSET 0xBE
#define HEADER_LOGO_WORD_OFFSET 0x06 /* part of the Nintendo logo */
#define HEADER_LOGO_WORD 0x51AE

void nor_games_load_table(void)
{
    config_flash_read(CONFIG_NOR_TABLE_OFFSET, (u16 *)g_nor_table, sizeof(g_nor_table) / 2);
}

void nor_games_save_table(void)
{
    config_flash_write_block(CONFIG_NOR_TABLE_OFFSET, g_nor_table, sizeof(g_nor_table));
}

void nor_games_init(void)
{
    static const nor_entry_t EMPTY;
    uint32_t used;

    nor_games_load_table();
    if (nor_games_scan(&used) != 0) {
        return;
    }
    for (unsigned i = 0; i < NOR_MAX_GAMES; i++) {
        if (memcmp(&g_nor_table[i], &EMPTY, sizeof(EMPTY)) != 0) {
            memset(g_nor_table, 0, sizeof(g_nor_table));
            nor_games_save_table();
            return;
        }
    }
}

/** Does the NOR window at @p address look like the start of a ROM? */
static bool looks_like_rom(u32 address)
{
    u16 reserved = *(vu16 *)(address + HEADER_RESERVED_OFFSET);
    u16 logo = *(vu16 *)(address + HEADER_LOGO_WORD_OFFSET);
    u8 low = reserved & 0xFF;
    return low == 0xCE || low == 0xCF || low == 0x00 || logo == HEADER_LOGO_WORD;
}

unsigned nor_games_scan(uint32_t *used_bytes)
{
    u32 address = NOR_BASE;
    u16 page = 0;
    unsigned count = 0;
    char title[16];

    *used_bytes = 0;
    REG_IME = 0;
    while (count < NOR_MAX_GAMES && looks_like_rom(address)) {
        if (*(vu8 *)(address + HEADER_FIXED_OFFSET) != HEADER_FIXED_VALUE) {
            break;
        }
        memcpy(title, (const void *)(address + ROM_HEADER_TITLE), sizeof(title));
        if (memcmp(title, g_nor_table[count].header_title, sizeof(title)) != 0) {
            break;
        }
        *used_bytes += g_nor_table[count].size;
        address += g_nor_table[count].size;
        count++;

        while (address >= NOR_WINDOW_END) {
            page += ROM_WINDOW_PAGE_STEP;
            if (page > 0x7000) {
                /* Reached the end of the chip. */
                omega_set_rom_page(KERNEL_ROM_PAGE);
                REG_IME = 1;
                return count;
            }
            omega_set_rom_page(KERNEL_ROM_PAGE + page);
            address -= ROM_WINDOW_SIZE;
        }
    }
    omega_set_rom_page(KERNEL_ROM_PAGE);
    REG_IME = 1;
    return count;
}

nor_write_result_t nor_game_write(const char *path, unsigned index, uint32_t offset,
                                  patch_context_t *patch, bool with_hooks,
                                  const progress_t *progress)
{
    FIL file;
    UINT read;
    nor_entry_t entry;

    if (nor_read_id() != NOR_DEVICE_ID_S98) {
        return NOR_WRITE_NO_CHIP;
    }
    if (index >= NOR_MAX_GAMES) {
        return NOR_WRITE_FULL;
    }
    if (f_open(&file, path, FA_READ) != FR_OK) {
        return NOR_WRITE_OPEN_FAILED;
    }

    uint32_t file_size = f_size(&file);
    uint32_t needed = ((file_size + NOR_BLOCK_SIZE - 1) / NOR_BLOCK_SIZE) * NOR_BLOCK_SIZE;
    bool extra_block = with_hooks && patch_payload_needs_extra_block(patch, file_size);
    if (extra_block) {
        needed += NOR_BLOCK_SIZE;
    }
    if (needed > NOR_GAMES_CAPACITY - offset) {
        f_close(&file);
        return NOR_WRITE_FULL;
    }

    memset(&entry, 0, sizeof(entry));
    f_lseek(&file, ROM_HEADER_TITLE);
    f_read(&file, entry.header_title, sizeof(entry.header_title), &read);
    entry.rom_page = (uint16_t)(offset >> 17);
    entry.size = needed;
    entry.has_hooks = with_hooks;
    entry.has_save_state = with_hooks && patch->settings.save_state_hook;
    char name[FF_LFN_BUF + 1];
    sd_long_name(path, name, sizeof(name));
    text_copy(entry.filename, sizeof(entry.filename), name);
    g_nor_table[index] = entry;

    nor_unprotect_all();

    uint32_t block;
    for (block = 0; block < file_size; block += NOR_BLOCK_SIZE) {
        progress_advance(progress, block, file_size);
        nor_erase_block(offset + block);
        f_lseek(&file, block);
        f_read(&file, g_scratch, NOR_BLOCK_SIZE, &read);
        if (with_hooks) {
            patch_scan_irq_references(patch, (const uint32_t *)g_scratch, NOR_BLOCK_SIZE, block);
            patch_apply_hooks_nor(patch, g_scratch, block);
        } else {
            patch_apply_clean_nor(patch, g_scratch, block);
        }
        omega_set_auto_save(patch->st.auto_save);
        nor_program_buffered(offset + block, g_scratch, NOR_BLOCK_SIZE);
    }
    f_close(&file);

    if (extra_block) {
        /* The payload lives in a block of its own after the ROM data. The
         * buffer still holds the last ROM block; only the payload matters. */
        nor_erase_block(offset + block);
        patch_apply_hooks_nor(patch, g_scratch, block);
        nor_program_buffered(offset + block, g_scratch, NOR_BLOCK_SIZE);
    }
    progress_advance(progress, file_size, file_size);

    nor_games_save_table();
    return NOR_WRITE_OK;
}

void nor_game_delete_last(unsigned count, uint32_t used_bytes)
{
    if (count == 0) {
        return;
    }
    nor_erase_block(used_bytes - g_nor_table[count - 1].size);
}

void nor_games_erase_all(void (*poll)(uint32_t tick))
{
    nor_erase_chip(poll);
    memset(g_nor_table, 0, sizeof(g_nor_table));
    nor_games_save_table();
}
