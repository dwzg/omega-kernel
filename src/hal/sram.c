/**
 * @file sram.c
 * @brief Cartridge SRAM access. See sram.h.
 */
#include "hal/sram.h"

#include "hal/platform.h"

void IWRAM_CODE sram_read(u32 address, u8 *data, u32 size)
{
    for (u32 i = 0; i < size; i++) {
        data[i] = *(vu8 *)(address + i);
    }
}

void IWRAM_CODE sram_write(u32 address, const u8 *data, u32 size)
{
    for (u32 i = 0; i < size; i++) {
        *(vu8 *)(address + i) = data[i];
    }
}

void IWRAM_CODE sram_flash_bank_switch(u8 bank)
{
    *(vu8 *)(SRAM_BASE + 0x5555) = 0xAA;
    *(vu8 *)(SRAM_BASE + 0x2AAA) = 0x55;
    *(vu8 *)(SRAM_BASE + 0x5555) = 0xB0;
    *(vu8 *)(SRAM_BASE + 0x0000) = bank;
}
