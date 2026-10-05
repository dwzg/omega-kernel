/**
 * @file omega.c
 * @brief EZ-FLASH OMEGA FPGA register access. See omega.h.
 *
 * Everything in this file runs from IWRAM: while an unlock sequence is in
 * progress, or after the ROM page has been switched to a game, the kernel's
 * own code in ROM is not readable.
 */
#include "hal/omega.h"

#include <gba_dma.h>

#include "hal/platform.h"
#include "hal/reset.h"

/* Addresses and magic values of the unlock sequence. */
#define UNLOCK_ADDR_1 0x09FE0000u
#define UNLOCK_ADDR_2 0x08000000u
#define UNLOCK_ADDR_3 0x08020000u
#define UNLOCK_ADDR_4 0x08040000u
#define LOCK_ADDR 0x09FC0000u
#define UNLOCK_KEY_A 0xD200u
#define UNLOCK_KEY_B 0x1500u

/* Buffer control register values used by omega_send_fat_map(). */
#define BUFFER_OFF 0u
#define BUFFER_WRITE 1u
#define BUFFER_COMMIT 3u

/* RegisterRamReset() flags used before jumping into a game: everything except
 * IWRAM (this code is running there). */
#define RESET_EWRAM (1u << 0)
#define RESET_PALETTE (1u << 2)
#define RESET_VRAM (1u << 3)
#define RESET_OAM (1u << 4)
#define RESET_SIO (1u << 5)
#define RESET_SOUND (1u << 6)
#define RESET_OTHER (1u << 7)
#define BOOT_RESET_FLAGS                                                                           \
    (RESET_EWRAM | RESET_PALETTE | RESET_VRAM | RESET_OAM | RESET_SIO | RESET_SOUND | RESET_OTHER)

static ALWAYS_INLINE void reg_write(u32 address, u16 value)
{
    *(vu16 *)address = value;
}

void IWRAM_CODE omega_write_register(u32 reg, u16 value)
{
    reg_write(UNLOCK_ADDR_1, UNLOCK_KEY_A);
    reg_write(UNLOCK_ADDR_2, UNLOCK_KEY_B);
    reg_write(UNLOCK_ADDR_3, UNLOCK_KEY_A);
    reg_write(UNLOCK_ADDR_4, UNLOCK_KEY_B);
    reg_write(reg, value);
    reg_write(LOCK_ADDR, UNLOCK_KEY_B);
}

void IWRAM_CODE omega_set_sd_mode(omega_sd_mode_t mode)
{
    omega_write_register(OMEGA_REG_SD_CONTROL, (u16)mode);
}

u16 IWRAM_CODE omega_read_data_port(void)
{
    return *(vu16 *)OMEGA_DATA_PORT;
}

void IWRAM_CODE omega_set_rom_page(u16 page)
{
    omega_write_register(OMEGA_REG_ROM_PAGE, page);
}

void IWRAM_CODE omega_set_psram_page(u16 page)
{
    omega_write_register(OMEGA_REG_PSRAM_PAGE, page);
}

void IWRAM_CODE omega_set_sram_page(u16 page)
{
    omega_write_register(OMEGA_REG_SRAM_PAGE, page);
}

void IWRAM_CODE omega_set_rtc_enabled(u16 enabled)
{
    omega_write_register(OMEGA_REG_RTC, enabled);
}

void IWRAM_CODE omega_set_auto_save(u16 enabled)
{
    omega_write_register(OMEGA_REG_AUTO_SAVE, enabled);
}

void IWRAM_CODE omega_set_spi_write(u16 enabled)
{
    omega_write_register(OMEGA_REG_SPI_WRITE, enabled);
}

u16 IWRAM_CODE omega_read_fpga_version(void)
{
    omega_write_register(OMEGA_REG_SPI_CONTROL, 1);
    u16 version = omega_read_data_port();
    omega_write_register(OMEGA_REG_SPI_CONTROL, 0);
    return version;
}

void IWRAM_CODE omega_send_fat_map(const u32 *buffer, omega_fat_map_mode_t mode)
{
    omega_write_register(OMEGA_REG_BUFFER_CONTROL, BUFFER_WRITE);
    dmaCopy(buffer, (void *)OMEGA_DATA_PORT, OMEGA_FAT_MAP_SIZE);
    if (mode == OMEGA_FAT_MAP_RAW) {
        omega_write_register(OMEGA_REG_BUFFER_CONTROL, BUFFER_OFF);
        return;
    }

    omega_write_register(OMEGA_REG_BUFFER_CONTROL, BUFFER_COMMIT);
    if (mode == OMEGA_FAT_MAP_UPLOAD) {
        omega_write_register(OMEGA_REG_BUFFER_CONTROL, BUFFER_OFF);
        return;
    }

    /* OMEGA_FAT_MAP_COPY_ROM: the FPGA reports 0x0000 until it starts copying,
     * then 0x0001 while the copy is in progress. */
    while (omega_read_data_port() == 0x0000) {
    }
    while (omega_read_data_port() == 0x0001) {
    }
    omega_write_register(OMEGA_REG_BUFFER_CONTROL, BUFFER_OFF);
}

void IWRAM_CODE omega_boot(u16 page, u16 rtc_enabled, u32 bios_boot)
{
    omega_set_rtc_enabled(rtc_enabled);
    omega_set_rom_page(page);
    reset_register_ram(BOOT_RESET_FLAGS);
    if (bios_boot) {
        reset_hard();
    } else {
        reset_soft();
    }
    for (;;) {
    }
}
