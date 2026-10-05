/**
 * @file fat_map.c
 * @brief FAT map construction. See fat_map.h.
 */
#include "loader/fat_map.h"

#include <gba_base.h>
#include <string.h>

#include "platform/attributes.h"

/* Exported by our copy of FatFs (see third_party/fatfs/README.md). */
DWORD Get_NextCluster(FFOBJID *obj, DWORD clst);
DWORD ClustToSect(FATFS *fs, DWORD clst);

#define WORD_ROM_RUNS (0x000 / 4)
#define WORD_ROM_SIZE (0x1F0 / 4)
#define WORD_COPY_MODE (0x1F4 / 4)
#define WORD_CLUSTER_SECTORS (0x1F8 / 4)
#define WORD_SAVE_INFO (0x1FC / 4)
#define WORD_SAVE_RUNS (0x200 / 4)
#define WORD_SAVE_STATE_RUNS (0x300 / 4)

#define END_OF_RUNS 0xFFFFFFFFu

uint32_t g_fat_map[0x400 / 4] PLATFORM_EWRAM __attribute__((aligned(4)));

void fat_map_reset(void)
{
    memset(g_fat_map, 0, sizeof(g_fat_map));
    g_fat_map[2] = END_OF_RUNS;
}

static void slot_range(fat_map_slot_t slot, uint32_t **begin, uint32_t **end)
{
    switch (slot) {
    case FAT_MAP_ROM:
        *begin = &g_fat_map[WORD_ROM_RUNS];
        *end = &g_fat_map[WORD_ROM_SIZE];
        break;
    case FAT_MAP_SAVE:
        *begin = &g_fat_map[WORD_SAVE_RUNS];
        *end = &g_fat_map[WORD_SAVE_STATE_RUNS];
        break;
    default:
        *begin = &g_fat_map[WORD_SAVE_STATE_RUNS];
        *end = &g_fat_map[0x400 / 4];
        break;
    }
}

fat_map_result_t fat_map_add_file(FATFS *fs, const char *path, fat_map_slot_t slot)
{
    FIL file;
    uint32_t *out;
    uint32_t *end;

    if (f_open(&file, path, FA_READ) != FR_OK) {
        return FAT_MAP_CANNOT_OPEN;
    }
    slot_range(slot, &out, &end);

    DWORD cluster = file.obj.sclust;
    DWORD previous = cluster;
    uint32_t clusters = 0;
    fat_map_result_t result = FAT_MAP_OK;

    *out++ = 0;
    *out++ = ClustToSect(fs, cluster);
    for (;;) {
        cluster = Get_NextCluster(&file.obj, cluster);
        clusters++;
        if (cluster != previous + 1) {
            /* A new run starts (or the chain ended; that entry is replaced
             * by the terminator below). */
            if (out + 2 > end) {
                result = FAT_MAP_TOO_FRAGMENTED;
                break;
            }
            *out++ = clusters * fs->csize;
            *out++ = ClustToSect(fs, cluster);
        }
        previous = cluster;
        /* Valid cluster numbers are 2 .. n_fatent - 1; anything else is the
         * end of the chain (or an error). */
        if (cluster < 2 || cluster >= fs->n_fatent) {
            break;
        }
    }
    if (result == FAT_MAP_OK) {
        out[-2] = END_OF_RUNS;
        out[-1] = 0;
    }
    f_close(&file);
    return result;
}

bool fat_map_save_present(void)
{
    return g_fat_map[WORD_SAVE_RUNS + 1] != 0;
}

void fat_map_set_parameters(uint32_t rom_size, fat_map_copy_mode_t copy_mode,
                            uint32_t sectors_per_cluster, save_mode_t save_mode, uint32_t save_size)
{
    g_fat_map[WORD_ROM_SIZE] = rom_size;
    g_fat_map[WORD_COPY_MODE] = copy_mode;
    g_fat_map[WORD_CLUSTER_SECTORS] = sectors_per_cluster;
    g_fat_map[WORD_SAVE_INFO] = ((uint32_t)save_mode << 24) | save_size;
}

void fat_map_set_copy_mode(fat_map_copy_mode_t copy_mode)
{
    g_fat_map[WORD_COPY_MODE] = copy_mode;
}
