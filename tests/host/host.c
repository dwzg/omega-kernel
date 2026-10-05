/**
 * @file host.c
 * @brief Frame buffer, assets and screenshots for the host build.
 */
#include "host.h"

#include <stdio.h>
#include <stdlib.h>

#include "core/crc32.h"
#include "gfx/font.h"
#include "gfx/gfx.h"

static uint16_t s_framebuffer[GFX_WIDTH * GFX_HEIGHT];

static uint8_t s_font_body[16384];
static uint8_t s_font_small[16384];
static uint8_t s_font_title[16384];

const font_t FONT_BODY = {s_font_body};
const font_t FONT_SMALL = {s_font_small};
const font_t FONT_TITLE = {s_font_title};

uint16_t *gfx_framebuffer(void)
{
    return s_framebuffer;
}

uint16_t *gfx_offscreen_buffer(void)
{
    static uint32_t buffer[GFX_WIDTH * GFX_OFFSCREEN_ROWS / 2];
    return (uint16_t *)buffer;
}

void gfx_init(void)
{
}

static bool load(const char *dir, const char *name, void *buf, size_t size, bool exact)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        return false;
    }
    size_t n = fread(buf, 1, size, f);
    bool too_big = fgetc(f) != EOF;
    fclose(f);
    if (too_big || (exact && n != size)) {
        fprintf(stderr, "%s: unexpected size\n", path);
        return false;
    }
    return true;
}

bool host_assets_load(const char *root)
{
    char fonts[512];
    snprintf(fonts, sizeof(fonts), "%s/assets/fonts", root);
    return load(fonts, "font_body.bin", s_font_body, sizeof(s_font_body), false) &&
           load(fonts, "font_small.bin", s_font_small, sizeof(s_font_small), false) &&
           load(fonts, "font_title.bin", s_font_title, sizeof(s_font_title), false);
}

bool host_save_screenshot(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    fprintf(f, "P6\n%d %d\n255\n", GFX_WIDTH, GFX_HEIGHT);
    for (int i = 0; i < GFX_WIDTH * GFX_HEIGHT; i++) {
        uint16_t c = s_framebuffer[i];
        unsigned char rgb[3] = {
            (unsigned char)((c & 31) * 255 / 31),
            (unsigned char)(((c >> 5) & 31) * 255 / 31),
            (unsigned char)(((c >> 10) & 31) * 255 / 31),
        };
        fwrite(rgb, 1, 3, f);
    }
    return fclose(f) == 0;
}

uint32_t host_screen_crc(void)
{
    return crc32(s_framebuffer, sizeof(s_framebuffer));
}
