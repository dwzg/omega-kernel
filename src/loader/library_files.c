/**
 * @file library_files.c
 * @brief Recently played list, cheat files and thumbnails. See library_files.h.
 */
#include "loader/library_files.h"

#include <stdio.h>
#include <string.h>

#include "core/game_file.h"
#include "core/path.h"
#include "loader/buffers.h"
#include "loader/game_info.h"
#include "loader/sd_paths.h"

/** Longest line accepted in the recently played file. */
#define RECENT_LINE_LEN 512
/** Bytes of the cheat index that are searched. */
#define CHEAT_INDEX_MAX 0x10000u
/** Thumbnail file: 54-byte BMP header followed by 120x80 16-bit pixels. */
#define THUMBNAIL_PIXEL_OFFSET 0x36
#define THUMBNAIL_FILE_SIZE 0x4B38
/** Where thumbnails are loaded inside ::g_scratch. */
#define THUMBNAIL_SCRATCH_OFFSET 0x10000

/* ----------------------------------------------------------- recent list -- */

void recent_file_load(recent_list_t *list)
{
    FIL file;
    char line[RECENT_LINE_LEN];

    recent_clear(list);
    if (f_open(&file, SD_FILE_RECENT, FA_READ) != FR_OK) {
        return;
    }
    while (f_gets(line, sizeof(line), &file) != NULL) {
        if (!recent_append_line(list, line)) {
            break;
        }
    }
    f_close(&file);
}

bool recent_file_save(const recent_list_t *list)
{
    FIL file;
    if (!sd_ensure_folder(SD_DIR_SAVES) ||
        f_open(&file, SD_FILE_RECENT, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return false;
    }
    for (unsigned i = 0; i < list->count; i++) {
        f_printf(&file, "%s\n", list->entries[i]);
    }
    f_close(&file);
    return true;
}

/* ------------------------------------------------------------- favorites -- */

void favorites_file_load(favorites_t *list)
{
    FIL file;
    char line[RECENT_LINE_LEN];

    favorites_clear(list);
    if (f_open(&file, SD_FILE_FAVORITES, FA_READ) != FR_OK) {
        return;
    }
    while (f_gets(line, sizeof(line), &file) != NULL && favorites_append_line(list, line)) {
    }
    f_close(&file);
}

bool favorites_file_save(const favorites_t *list)
{
    FIL file;
    if (!sd_ensure_folder(SD_DIR_SAVES) ||
        f_open(&file, SD_FILE_FAVORITES, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return false;
    }
    for (unsigned i = 0; i < list->count; i++) {
        f_printf(&file, "%s\n", list->entries[i]);
    }
    f_close(&file);
    return true;
}

/* ----------------------------------------------------------- cheat files -- */

static bool file_exists(const char *path)
{
    FILINFO info;
    return f_stat(path, &info) == FR_OK;
}

bool cheat_file_find(const char *game_path, char *out, size_t out_size)
{
    game_info_t info = {0};
    uint32_t game_id = 0;

    /* A game without a code (homebrew) has no cheats. If the header can't
     * be read, only a cheat file named after the game is looked for. */
    if (game_info_read(game_path, &info)) {
        memcpy(&game_id, info.game_code, 4);
        if (game_id == 0) {
            return false;
        }
    }

    /* 1. A cheat file named after the game. */
    char name[FF_LFN_BUF + 1];
    sd_long_name(game_path, name, sizeof(name));
    if (sd_companion_path(out, out_size, SD_DIR_CHEATS, name, "cht") && file_exists(out)) {
        return true;
    }

    /* 2. The shared library, through the game code index. */
    if (game_id == 0) {
        return false;
    }
    FIL index;
    UINT read = 0;
    if (f_open(&index, SD_FILE_CHEAT_INDEX, FA_READ) != FR_OK) {
        return false;
    }
    uint32_t *pairs = (uint32_t *)g_scratch;
    f_read(&index, pairs, CHEAT_INDEX_MAX, &read);
    f_close(&index);

    for (uint32_t i = 0; i + 1 < read / 4; i += 2) {
        if (pairs[i] != game_id) {
            continue;
        }
        char folder[32];
        char file[32];
        cht_library_paths(pairs[i + 1], folder, sizeof(folder), file, sizeof(file));
        return path_join(out, out_size, folder, file) && file_exists(out);
    }
    return false;
}

static void reader_rewind(void *ctx)
{
    cheat_file_t *cf = ctx;
    f_lseek(&cf->file, 0);
    cf->buffered = cf->pos = 0;
}

/** Read one line of raw bytes, like f_gets() but without any conversion. */
static bool reader_read_line(void *ctx, char *buf, size_t size)
{
    cheat_file_t *cf = ctx;
    size_t n = 0;
    while (n + 1 < size) {
        if (cf->pos == cf->buffered) {
            UINT got = 0;
            f_read(&cf->file, cf->buffer, sizeof(cf->buffer), &got);
            cf->buffered = got;
            cf->pos = 0;
            if (got == 0) {
                break; /* end of file */
            }
        }
        char c = cf->buffer[cf->pos++];
        buf[n++] = c;
        if (c == '\n') {
            break;
        }
    }
    buf[n] = '\0';
    return n > 0;
}

bool cheat_file_open(cheat_file_t *cf, const char *path)
{
    if (f_open(&cf->file, path, FA_READ) != FR_OK) {
        return false;
    }
    cf->buffered = cf->pos = 0;
    cf->reader.ctx = cf;
    cf->reader.rewind = reader_rewind;
    cf->reader.read_line = reader_read_line;
    return true;
}

void cheat_file_close(cheat_file_t *cf)
{
    f_close(&cf->file);
}

/* ------------------------------------------------------------ thumbnails -- */

const uint16_t *thumbnail_load(const char game_code[4])
{
    char path[48];
    FIL file;
    UINT read = 0;
    uint8_t *dst = g_scratch + THUMBNAIL_SCRATCH_OFFSET;

    snprintf(path, sizeof(path), SD_DIR_THUMBNAILS "/%c/%c/%c%c%c%c.bmp", game_code[0],
             game_code[1], game_code[0], game_code[1], game_code[2], game_code[3]);
    if (f_open(&file, path, FA_READ) != FR_OK) {
        return NULL;
    }
    f_read(&file, dst, THUMBNAIL_FILE_SIZE, &read);
    f_close(&file);
    if (read < THUMBNAIL_FILE_SIZE - 2) {
        return NULL;
    }
    return (const uint16_t *)(dst + THUMBNAIL_PIXEL_OFFSET);
}
