/**
 * @file omega.h
 * @brief EZ-FLASH OMEGA FPGA register interface.
 *
 * The cartridge FPGA is controlled through "registers" that live in the
 * cartridge ROM address space. A register write is only accepted when it is
 * wrapped in an unlock sequence:
 *
 * @code
 *   *(vu16 *)0x09FE0000 = 0xD200;
 *   *(vu16 *)0x08000000 = 0x1500;
 *   *(vu16 *)0x08020000 = 0xD200;
 *   *(vu16 *)0x08040000 = 0x1500;
 *   *(vu16 *)REGISTER   = value;
 *   *(vu16 *)0x09FC0000 = 0x1500;
 * @endcode
 *
 * Because the sequence temporarily changes what the ROM bus returns, every
 * function that issues it executes from IWRAM (::IWRAM_CODE).
 *
 * Register map (see docs/hardware.md for details):
 *
 * | Address      | Name            | Meaning                                         |
 * |--------------|-----------------|-------------------------------------------------|
 * | `0x09400000` | SD control      | 0 = off, 1 = on, 3 = read status                 |
 * | `0x09420000` | Buffer control  | access to the FPGA's FAT map buffer              |
 * | `0x09600000` | SD address low  | sector number bits 0-15                          |
 * | `0x09620000` | SD address high | sector number bits 16-31                         |
 * | `0x09640000` | SD count        | sectors to transfer (bit 15 = write)             |
 * | `0x09660000` | SPI control     | 1 = read FPGA version / SPI data                 |
 * | `0x09680000` | SPI write       | 1 = program FPGA configuration flash             |
 * | `0x096A0000` | RTC             | 1 = RTC visible to games                         |
 * | `0x096C0000` | Auto-save       | 1 = FPGA saves SRAM to SD automatically          |
 * | `0x09860000` | PSRAM page      | which 8 MiB of PSRAM appears at 0x08800000       |
 * | `0x09880000` | ROM page        | which memory appears at 0x08000000 / 0x09000000  |
 * | `0x09C00000` | SRAM page       | which 64 KiB SRAM bank appears at 0x0E000000     |
 * | `0x09E00000` | Data port       | SD data, status and SPI reads                    |
 */
#ifndef HAL_OMEGA_H
#define HAL_OMEGA_H

#include <gba_base.h>

/** @name Register addresses */
/**@{*/
#define OMEGA_REG_SD_CONTROL 0x09400000u
#define OMEGA_REG_BUFFER_CONTROL 0x09420000u
#define OMEGA_REG_SD_ADDRESS_LOW 0x09600000u
#define OMEGA_REG_SD_ADDRESS_HIGH 0x09620000u
#define OMEGA_REG_SD_COUNT 0x09640000u
#define OMEGA_REG_SPI_CONTROL 0x09660000u
#define OMEGA_REG_SPI_WRITE 0x09680000u
#define OMEGA_REG_RTC 0x096A0000u
#define OMEGA_REG_AUTO_SAVE 0x096C0000u
#define OMEGA_REG_PSRAM_PAGE 0x09860000u
#define OMEGA_REG_ROM_PAGE 0x09880000u
#define OMEGA_REG_SRAM_PAGE 0x09C00000u
#define OMEGA_DATA_PORT 0x09E00000u
/**@}*/

/** SD control register values. */
typedef enum {
    OMEGA_SD_OFF = 0,        /**< SD interface disabled, ROM bus normal. */
    OMEGA_SD_ON = 1,         /**< SD interface enabled. */
    OMEGA_SD_READ_STATUS = 3 /**< Data port returns the SD state machine status. */
} omega_sd_mode_t;

/** How omega_send_fat_map() hands the FAT map to the FPGA. */
typedef enum {
    /** Upload the map and let the FPGA copy the ROM into PSRAM; waits until done. */
    OMEGA_FAT_MAP_COPY_ROM = 0,
    /** Upload the map only (the ROM is already in PSRAM or NOR). */
    OMEGA_FAT_MAP_UPLOAD = 1,
    /** Upload a raw 1 KiB buffer (used to stream firmware updates). */
    OMEGA_FAT_MAP_RAW = 2
} omega_fat_map_mode_t;

/** Size of the FPGA's FAT map buffer in bytes. */
#define OMEGA_FAT_MAP_SIZE 0x400u

/**
 * @brief Write one FPGA register using the unlock sequence.
 * @param reg   Register address (one of the OMEGA_REG_* constants).
 * @param value Value to write.
 */
void IWRAM_CODE omega_write_register(u32 reg, u16 value);

/** @brief Select the SD interface mode. */
void IWRAM_CODE omega_set_sd_mode(omega_sd_mode_t mode);

/** @brief Read the 16-bit word currently presented on the FPGA data port. */
u16 IWRAM_CODE omega_read_data_port(void);

/**
 * @brief Map memory into the cartridge ROM space.
 *
 * Page ::KERNEL_ROM_PAGE shows the kernel at 0x08000000 and NOR at 0x09000000;
 * ::PSRAM_ROM_PAGE shows PSRAM at 0x08000000 (used to boot a loaded game).
 */
void IWRAM_CODE omega_set_rom_page(u16 page);

/** @brief Select which 8 MiB of PSRAM appears at ::PSRAM_BASE. */
void IWRAM_CODE omega_set_psram_page(u16 page);

/** @brief Select which 64 KiB bank of SRAM appears at ::SRAM_BASE. */
void IWRAM_CODE omega_set_sram_page(u16 page);

/** @brief Enable (1) or disable (0) the cartridge RTC for games. */
void IWRAM_CODE omega_set_rtc_enabled(u16 enabled);

/** @brief Enable (1) or disable (0) the FPGA's automatic save write-back. */
void IWRAM_CODE omega_set_auto_save(u16 enabled);

/** @brief Read the FPGA firmware version. */
u16 IWRAM_CODE omega_read_fpga_version(void);

/**
 * @brief Upload the 1 KiB FAT map (or a raw buffer) to the FPGA.
 * @param buffer @ref OMEGA_FAT_MAP_SIZE bytes, 32-bit aligned.
 * @param mode   What the FPGA should do with it.
 */
void IWRAM_CODE omega_send_fat_map(const u32 *buffer, omega_fat_map_mode_t mode);

/** @brief Allow (1) or block (0) writes to the FPGA configuration flash over SPI. */
void IWRAM_CODE omega_set_spi_write(u16 enabled);

/**
 * @brief Switch the cartridge to a game and reset the GBA into it.
 *
 * Applies the in-game RTC setting, maps @p page, clears RAM and registers
 * with the BIOS and jumps to the game. Never returns.
 *
 * @param page          ROM page to boot (::PSRAM_ROM_PAGE or a NOR page).
 * @param rtc_enabled   Whether the game may use the cartridge RTC.
 * @param bios_boot     true: reset through the BIOS boot sequence (logo,
 *                      multiboot-capable); false: jump straight to the game.
 */
void IWRAM_CODE omega_boot(u16 page, u16 rtc_enabled, u32 bios_boot) __attribute__((noreturn));

#endif /* HAL_OMEGA_H */
