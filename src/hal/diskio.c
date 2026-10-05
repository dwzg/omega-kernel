/**
 * @file diskio.c
 * @brief FatFs low-level disk I/O glue for the OMEGA's microSD slot.
 *
 * The card is FatFs physical drive 0 and the only drive. Initialisation is
 * done by the FPGA, so status/initialize/ioctl have nothing to do.
 */
#include "ff.h"
/* diskio.h needs the types from ff.h. */
#include "diskio.h"

#include "core/datetime.h"
#include "hal/rtc.h"
#include "hal/sd.h"

DSTATUS disk_status(BYTE pdrv)
{
    (void)pdrv;
    return 0;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    (void)pdrv;
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    (void)pdrv;
    return sd_read_sectors(sector, (u16)count, buff) ? RES_ERROR : RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    (void)pdrv;
    return sd_write_sectors(sector, (u16)count, buff) ? RES_ERROR : RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    (void)pdrv;
    (void)cmd;
    (void)buff;
    return RES_OK;
}

DWORD get_fattime(void)
{
    u8 bcd[RTC_DATETIME_BYTES];
    datetime_t now;

    rtc_read_datetime(bcd);
    datetime_from_bcd(&now, bcd);
    return datetime_to_fat(&now);
}
