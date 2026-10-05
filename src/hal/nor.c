/**
 * @file nor.c
 * @brief S98 NOR flash driver. See nor.h.
 */
#include "hal/nor.h"

#include <gba_interrupt.h>

#include "hal/omega.h"
#include "hal/platform.h"

#define NOR_WORD(offset) (*(vu16 *)(NOR_BASE + (offset)))
#define CMD_ADDR_1 (0x555u * 2)
#define CMD_ADDR_2 (0x2AAu * 2)

/** Size of the small sectors at both ends of the chip. */
#define NOR_SMALL_SECTOR 0x8000u
/** Offset of the last 128 KiB block (also made of small sectors). */
#define NOR_LAST_BLOCK (NOR_TOTAL_SIZE - NOR_BLOCK_SIZE)

/**
 * Map the 8 MiB window that contains @p offset and return the offset inside
 * that window.
 */
static u32 map_window(u32 offset)
{
    u16 page = KERNEL_ROM_PAGE;
    while (offset >= ROM_WINDOW_SIZE) {
        offset -= ROM_WINDOW_SIZE;
        page += ROM_WINDOW_PAGE_STEP;
    }
    omega_set_rom_page(page);
    return offset;
}

static void unlock(void)
{
    NOR_WORD(CMD_ADDR_1) = 0xAA;
    NOR_WORD(CMD_ADDR_2) = 0x55;
}

/** Wait until two consecutive reads agree (toggle bit stopped). */
static void wait_ready(u32 offset)
{
    vu16 v1, v2;
    do {
        v1 = NOR_WORD(offset);
        v2 = NOR_WORD(offset);
    } while (v1 != v2);
}

static void erase_sector(u32 window_offset)
{
    unlock();
    NOR_WORD(CMD_ADDR_1) = 0x80;
    unlock();
    NOR_WORD(window_offset) = 0x30;
    wait_ready(window_offset);
}

u16 nor_read_id(void)
{
    NOR_WORD(0) = 0xF0;
    unlock();
    NOR_WORD(CMD_ADDR_1) = 0x90;
    u16 id = NOR_WORD(0xE * 2);
    nor_reset();
    return id;
}

void nor_reset(void)
{
    NOR_WORD(0) = 0xF0;
}

void nor_erase_block(u32 offset)
{
    u32 window_offset = map_window(offset);
    nor_reset();

    if (offset == 0 || offset == NOR_LAST_BLOCK) {
        for (u32 sector = 0; sector < NOR_BLOCK_SIZE; sector += NOR_SMALL_SECTOR) {
            erase_sector(window_offset + sector);
        }
    } else {
        erase_sector(window_offset);
    }
    omega_set_rom_page(KERNEL_ROM_PAGE);
}

void nor_program(u32 offset, const u8 *data, u32 size)
{
    const u16 *src = (const u16 *)data;
    u32 window_offset = map_window(offset);
    nor_reset();

    for (u32 i = 0; i < size / 2; i++) {
        unlock();
        NOR_WORD(CMD_ADDR_1) = 0xA0;
        NOR_WORD(window_offset + i * 2) = src[i];
        wait_ready(window_offset + i * 2);
    }
    omega_set_rom_page(KERNEL_ROM_PAGE);
}

void IWRAM_CODE nor_program_buffered(u32 offset, const u8 *data, u32 size)
{
    const u16 *src = (const u16 *)data;
    u16 page = KERNEL_ROM_PAGE;
    vu16 v1, v2;

    /* map_window()/wait_ready() live in ROM, which is fine for them but this
     * hot loop is kept self-contained in IWRAM for speed. */
    while (offset >= ROM_WINDOW_SIZE) {
        offset -= ROM_WINDOW_SIZE;
        page += ROM_WINDOW_PAGE_STEP;
    }
    omega_set_rom_page(page);
    NOR_WORD(0) = 0xF0;

    for (u32 chunk = 0; chunk < size / 32; chunk++) {
        u32 base = offset + chunk * 32;
        NOR_WORD(CMD_ADDR_1) = 0xAA;
        NOR_WORD(CMD_ADDR_2) = 0x55;
        NOR_WORD(base) = 0x25; /* write to buffer */
        NOR_WORD(base) = 15;   /* word count - 1 */
        for (u32 i = 0; i < 16; i++) {
            NOR_WORD(base + 2 * i) = src[chunk * 16 + i];
        }
        NOR_WORD(base) = 0x29; /* program buffer to flash */
        do {
            v1 = NOR_WORD(base + 0xF * 2);
            v2 = NOR_WORD(base + 0xF * 2);
        } while (v1 != v2);
    }
    omega_set_rom_page(KERNEL_ROM_PAGE);
}

void nor_unprotect_all(void)
{
    NOR_WORD(0) = 0xF0;
    unlock();
    NOR_WORD(CMD_ADDR_1) = 0xC0; /* enter PPB command set */
    NOR_WORD(0) = 0x80;
    NOR_WORD(0) = 0x30; /* erase all PPBs */
    for (int polls = 0x15000; polls; polls--) {
        (void)NOR_WORD(0x5C0000);
    }
    NOR_WORD(0) = 0x90; /* exit PPB command set */
    NOR_WORD(0) = 0x00;
}

void nor_erase_chip(void (*poll)(u32 tick))
{
    vu16 v1, v2;
    u32 tick = 0;

    REG_IME = 0;
    unlock();
    NOR_WORD(CMD_ADDR_1) = 0x80;
    unlock();
    NOR_WORD(CMD_ADDR_1) = 0x10;
    do {
        if (poll) {
            poll(tick++);
        }
        v1 = NOR_WORD(0);
        v2 = NOR_WORD(0);
    } while (v1 != v2);
    REG_IME = 1;
}
