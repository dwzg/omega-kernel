/**
 * @file firmware.c
 * @brief FPGA firmware update. See firmware.h.
 *
 * The image is sent in 256-byte pages through the FAT map buffer: word 0 is
 * the target address in the configuration flash, words 1-64 the data. Each
 * page is committed with the SPI write register and the FPGA reports
 * completion by returning 0 on the data port.
 */
#include "loader/firmware.h"

#include <gba_dma.h>
#include <string.h>

#include "core/crc32.h"
#include "hal/omega.h"
#include "loader/fat_map.h"
#include "omega_fpga_bin.h"

/** Bytes per configuration flash page. */
#define PAGE_SIZE 256u
/** Flash address of the FPGA image. */
#define IMAGE_FLASH_ADDRESS 0x40000u

bool firmware_update_available(uint16_t version)
{
    return version < FIRMWARE_BUNDLED_VERSION || version == FIRMWARE_TEST_VERSION;
}

bool firmware_image_valid(void)
{
    return crc32(omega_fpga_bin, omega_fpga_bin_size) == FIRMWARE_BUNDLED_CRC32;
}

void firmware_write(const progress_t *progress)
{
    uint8_t *page_data = (uint8_t *)&g_fat_map[1];

    omega_set_spi_write(0);
    for (uint32_t offset = 0; offset < omega_fpga_bin_size; offset += PAGE_SIZE) {
        progress_advance(progress, offset, omega_fpga_bin_size);

        g_fat_map[0] = IMAGE_FLASH_ADDRESS + offset;
        uint32_t remaining = omega_fpga_bin_size - offset;
        if (remaining < PAGE_SIZE) {
            /* Last page: pad with the erased-flash value. */
            memset(page_data, 0xFF, PAGE_SIZE);
            memcpy(page_data, omega_fpga_bin + offset, remaining);
        } else {
            dmaCopy(omega_fpga_bin + offset, page_data, PAGE_SIZE);
        }
        omega_send_fat_map(g_fat_map, OMEGA_FAT_MAP_RAW);

        omega_set_spi_write(1);
        while (omega_read_data_port() != 0) {
        }
        omega_set_spi_write(0);
    }
    progress_advance(progress, omega_fpga_bin_size, omega_fpga_bin_size);
}
