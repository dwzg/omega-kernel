/**
 * @file patch_platform.c
 * @brief Patch engine services on the GBA. See patch_platform.h.
 */
#include "loader/patch_platform.h"

#include "hal/omega.h"
#include "hal/platform.h"

/** Write halfwords to PSRAM, switching the 8 MiB window as needed. */
static void write_psram(uint32_t rom_offset, const void *data, uint32_t size)
{
    const u16 *src = data;
    for (uint32_t i = 0; i < size / 2; i++) {
        uint32_t offset = rom_offset + i * 2;
        u16 page = (u16)((offset / ROM_WINDOW_SIZE) * ROM_WINDOW_PAGE_STEP);
        if (i == 0 || offset % ROM_WINDOW_SIZE == 0) {
            omega_set_psram_page(page);
        }
        *(vu16 *)(PSRAM_BASE + offset % ROM_WINDOW_SIZE) = src[i];
    }
    omega_set_psram_page(0);
}

static const patch_platform_t PLATFORM = {
    .write_psram = write_psram,
    .work = (uint8_t *)VRAM_WORK_BUFFER,
    .work_size = VRAM_WORK_BUFFER_END - VRAM_WORK_BUFFER,
};

const patch_platform_t *patch_platform_gba(void)
{
    return &PLATFORM;
}
