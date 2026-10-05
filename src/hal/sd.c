/**
 * @file sd.c
 * @brief microSD sector transfers. See sd.h.
 *
 * Protocol: write the sector number and count into the SD address registers
 * (inside one unlock sequence), switch the SD interface to "read status",
 * poll the data port until it stops reporting busy (0xEEE1), switch back to
 * "on" and copy the data from / to the data port window.
 */
#include "hal/sd.h"

#include <gba_dma.h>

#include "hal/omega.h"
#include "hal/timing.h"

/** Data port value while the card is still busy. */
#define SD_BUSY 0xEEE1u
/** Polls of the data port before a transfer is considered timed out. */
#define SD_TIMEOUT_POLLS 0x100000u
/** Sectors per FPGA transfer. */
#define SD_CHUNK_SECTORS 4u
/** Attempts per chunk before a read is reported as failed. */
#define SD_READ_ATTEMPTS 2u
/** Bit in the count register that marks a write. */
#define SD_COUNT_WRITE 0x8000u

/** @return true if the card answered, false on time-out. */
static bool IWRAM_CODE wait_for_card(void)
{
    for (u32 polls = 0; polls <= SD_TIMEOUT_POLLS; polls++) {
        if (omega_read_data_port() != SD_BUSY) {
            return true;
        }
    }
    return false;
}

/** Program sector number and count; the three writes share one unlock. */
static void IWRAM_CODE start_transfer(u32 sector, u16 count)
{
    *(vu16 *)0x09FE0000 = 0xD200;
    *(vu16 *)0x08000000 = 0x1500;
    *(vu16 *)0x08020000 = 0xD200;
    *(vu16 *)0x08040000 = 0x1500;
    *(vu16 *)OMEGA_REG_SD_ADDRESS_LOW = (u16)(sector & 0x0000FFFFu);
    *(vu16 *)OMEGA_REG_SD_ADDRESS_HIGH = (u16)((sector & 0xFFFF0000u) >> 16);
    *(vu16 *)OMEGA_REG_SD_COUNT = count;
    *(vu16 *)0x09FC0000 = 0x1500;
}

u32 IWRAM_CODE sd_read_sectors(u32 sector, u16 count, u8 *buffer)
{
    omega_set_sd_mode(OMEGA_SD_ON);

    for (u16 done = 0; done < count; done += SD_CHUNK_SECTORS) {
        u16 remaining = (u16)(count - done);
        u16 blocks = remaining > SD_CHUNK_SECTORS ? SD_CHUNK_SECTORS : remaining;
        u32 attempts = SD_READ_ATTEMPTS;
        bool answered;

        for (;;) {
            start_transfer(sector + done, blocks);
            omega_set_sd_mode(OMEGA_SD_READ_STATUS);
            answered = wait_for_card();
            omega_set_sd_mode(OMEGA_SD_ON);
            if (answered || --attempts == 0) {
                break;
            }
            delay_loop(5000);
        }

        if (!answered) {
            /* Never hand stale buffer contents to the file system. */
            omega_set_sd_mode(OMEGA_SD_OFF);
            return 1;
        }
        dmaCopy((void *)OMEGA_DATA_PORT, buffer + done * SD_SECTOR_SIZE, blocks * SD_SECTOR_SIZE);
    }

    omega_set_sd_mode(OMEGA_SD_OFF);
    return 0;
}

u32 IWRAM_CODE sd_write_sectors(u32 sector, u16 count, const u8 *buffer)
{
    omega_set_sd_mode(OMEGA_SD_ON);
    omega_set_sd_mode(OMEGA_SD_READ_STATUS);

    for (u16 done = 0; done < count; done += SD_CHUNK_SECTORS) {
        u16 remaining = (u16)(count - done);
        u16 blocks = remaining > SD_CHUNK_SECTORS ? SD_CHUNK_SECTORS : remaining;

        dmaCopy(buffer + done * SD_SECTOR_SIZE, (void *)OMEGA_DATA_PORT, blocks * SD_SECTOR_SIZE);
        start_transfer(sector + done, (u16)(SD_COUNT_WRITE + blocks));
        (void)wait_for_card();
    }

    delay_loop(3000);
    omega_set_sd_mode(OMEGA_SD_OFF);
    return 0;
}
