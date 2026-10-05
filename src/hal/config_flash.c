/**
 * @file config_flash.c
 * @brief S71 flash storage for settings and the NOR game table. See config_flash.h.
 *
 * Runs from IWRAM: while the chip is in command mode the kernel image in the
 * same chip cannot be read.
 */
#include "hal/config_flash.h"

#include "hal/platform.h"

#define S71_WORD(offset) (*(vu16 *)(FLASH_S71_BASE + (offset)))
#define CMD_ADDR_1 (0x555u * 2)
#define CMD_ADDR_2 (0x2AAu * 2)

static ALWAYS_INLINE void unlock(void)
{
    S71_WORD(CMD_ADDR_1) = 0xAA;
    S71_WORD(CMD_ADDR_2) = 0x55;
}

static ALWAYS_INLINE void wait_ready(u32 offset)
{
    vu16 v1, v2;
    do {
        v1 = S71_WORD(offset);
        v2 = S71_WORD(offset);
    } while (v1 != v2);
}

u16 IWRAM_CODE config_flash_read_id(void)
{
    S71_WORD(0) = 0xF0;
    unlock();
    S71_WORD(CMD_ADDR_1) = 0x90;
    u16 id = S71_WORD(0xE * 2);
    S71_WORD(0) = 0xF0;
    return id;
}

u16 IWRAM_CODE config_flash_read16(u32 offset)
{
    return S71_WORD(offset);
}

void IWRAM_CODE config_flash_read(u32 offset, u16 *dst, u32 count)
{
    for (u32 i = 0; i < count; i++) {
        dst[i] = S71_WORD(offset + i * 2);
    }
}

void IWRAM_CODE config_flash_write_block(u32 offset, const void *src, u32 size)
{
    const u16 *buf = src;
    u16 id = config_flash_read_id();

    S71_WORD(0) = 0xF0;

    /* Erase the block. */
    unlock();
    S71_WORD(CMD_ADDR_1) = 0x80;
    unlock();
    S71_WORD(offset) = 0x30;
    wait_ready(offset);

    if (id == CONFIG_FLASH_ID_PL064) {
        /* No write buffer: program one halfword at a time. */
        for (u32 i = 0; i < size / 2; i++) {
            unlock();
            S71_WORD(CMD_ADDR_1) = 0xA0;
            S71_WORD(offset + i * 2) = buf[i];
            wait_ready(offset + i * 2);
        }
    } else {
        /* Program 32-byte write buffers. */
        for (u32 chunk = 0; chunk < size / 32; chunk++) {
            u32 base = offset + chunk * 32;
            unlock();
            S71_WORD(base) = 0x25;
            S71_WORD(base) = 15;
            for (u32 i = 0; i <= 15; i++) {
                S71_WORD(base + 2 * i) = buf[chunk * 16 + i];
            }
            S71_WORD(base) = 0x29;
            wait_ready(base);
        }
    }

    S71_WORD(0) = 0xF0;
}
