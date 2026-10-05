/**
 * @file sd.h
 * @brief Sector access to the microSD card through the OMEGA FPGA.
 *
 * Used by the FatFs glue (diskio.c). Sectors are 512 bytes; transfers are
 * split into chunks of up to four sectors, the size of the FPGA buffer.
 */
#ifndef HAL_SD_H
#define HAL_SD_H

#include <gba_base.h>

/** Size of one SD sector in bytes. */
#define SD_SECTOR_SIZE 512u

/**
 * @brief Read sectors from the card.
 * @param sector First sector (LBA).
 * @param count  Number of sectors.
 * @param buffer Destination, @p count * 512 bytes.
 * @return 0 on success, 1 if the card did not answer after retrying.
 */
u32 IWRAM_CODE sd_read_sectors(u32 sector, u16 count, u8 *buffer);

/**
 * @brief Write sectors to the card.
 * @param sector First sector (LBA).
 * @param count  Number of sectors.
 * @param buffer Source, @p count * 512 bytes.
 * @return Always 0. The original firmware protocol has no reliable write
 *         acknowledgement, so write time-outs are not reported.
 */
u32 IWRAM_CODE sd_write_sectors(u32 sector, u16 count, const u8 *buffer);

#endif /* HAL_SD_H */
