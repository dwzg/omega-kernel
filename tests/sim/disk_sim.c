/**
 * @file disk_sim.c
 * @brief FatFs disk I/O on an SD card image held in memory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"
/* diskio.h needs the types from ff.h. */
#include "diskio.h"
#include "sim.h"

#define SECTOR 512

static unsigned char *s_image;
static size_t s_size;

void sim_disk_open(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "sim: cannot open SD image %s\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    s_size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    free(s_image);
    s_image = malloc(s_size);
    if (!s_image || fread(s_image, 1, s_size, f) != s_size) {
        fprintf(stderr, "sim: cannot read SD image %s\n", path);
        exit(1);
    }
    fclose(f);
}

DSTATUS disk_status(BYTE pdrv)
{
    return s_image ? 0 : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    return disk_status(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if ((sector + count) * SECTOR > s_size) {
        return RES_PARERR;
    }
    memcpy(buff, s_image + sector * SECTOR, count * SECTOR);
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if ((sector + count) * SECTOR > s_size) {
        return RES_PARERR;
    }
    memcpy(s_image + sector * SECTOR, buff, count * SECTOR);
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    return cmd == CTRL_SYNC ? RES_OK : RES_PARERR;
}

DWORD get_fattime(void)
{
    /* 2026-10-05 09:41:00 */
    return ((DWORD)(2026 - 1980) << 25) | (10u << 21) | (5u << 16) | (9u << 11) | (41u << 5);
}
