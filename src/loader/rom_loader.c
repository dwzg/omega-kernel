/**
 * @file rom_loader.c
 * @brief Software ROM copies into PSRAM. See rom_loader.h.
 */
#include "loader/rom_loader.h"

#include <gba_dma.h>

#include "ff.h"
#include "hal/omega.h"
#include "hal/platform.h"
#include "loader/buffers.h"

/** Bytes copied per step (one scratch buffer). */
#define CHUNK SCRATCH_SIZE

/**
 * Copy @p size bytes from ::g_scratch to PSRAM at ROM offset @p offset,
 * splitting the copy where it crosses an 8 MiB window boundary.
 */
static void copy_to_psram(uint32_t offset, uint32_t size)
{
    const uint8_t *src = g_scratch;
    while (size > 0) {
        u16 page = (u16)((offset / ROM_WINDOW_SIZE) * ROM_WINDOW_PAGE_STEP);
        uint32_t in_window = offset % ROM_WINDOW_SIZE;
        uint32_t part = ROM_WINDOW_SIZE - in_window;
        if (part > size) {
            part = size;
        }
        omega_set_psram_page(page);
        dmaCopy(src, (void *)(PSRAM_BASE + in_window), part);
        src += part;
        offset += part;
        size -= part;
    }
    omega_set_psram_page(0);
}

/** Stream @p file into PSRAM starting at ROM offset @p base. */
static void stream_file(FIL *file, uint32_t base, patch_context_t *scan, const progress_t *progress)
{
    UINT read;
    uint32_t size = f_size(file);

    f_lseek(file, 0);
    for (uint32_t offset = 0; offset < size; offset += CHUNK) {
        progress_advance(progress, offset, size);
        f_read(file, g_scratch, CHUNK, &read);
        if (scan) {
            patch_scan_irq_references(scan, (const uint32_t *)g_scratch, CHUNK, offset);
        }
        copy_to_psram(base + offset, CHUNK);
    }
    progress_advance(progress, size, size);
}

bool rom_load_to_psram(const char *path, patch_context_t *scan, const progress_t *progress)
{
    FIL file;
    if (f_open(&file, path, FA_READ) != FR_OK) {
        return false;
    }
    stream_file(&file, 0, scan, progress);
    f_close(&file);
    return true;
}

bool rom_read_last_block(const char *path, uint32_t rom_size)
{
    FIL file;
    UINT read;

    if (rom_size == 0 || f_open(&file, path, FA_READ) != FR_OK) {
        return false;
    }
    f_lseek(&file, (rom_size - 1) & ~(SCRATCH_SIZE - 1));
    f_read(&file, g_scratch, SCRATCH_SIZE, &read);
    f_close(&file);
    return true;
}
