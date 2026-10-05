/**
 * @file platform_gba.c
 * @brief Platform services on the GBA. See platform/platform.h.
 */
#include "platform/platform.h"

#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_video.h>

#include "gfx/gfx.h"
#include "hal/config_flash.h"
#include "hal/omega.h"
#include "hal/platform.h"
#include "hal/rtc.h"

uint16_t *gfx_framebuffer(void)
{
    return VRAM_BASE;
}

_Static_assert(GFX_WIDTH *GFX_OFFSCREEN_ROWS * 2 <= VRAM_WORK_BUFFER_END - VRAM_WORK_BUFFER,
               "off-screen buffer must fit into spare VRAM");

/* Spare VRAM after the mode 3 frame buffer. While a game is being started it
 * holds the patch payloads instead (see patch_platform.c); the menu does not
 * draw lists then. */
uint16_t *gfx_offscreen_buffer(void)
{
    return (uint16_t *)VRAM_WORK_BUFFER;
}

void gfx_init(void)
{
    SetMode(MODE_3 | BG2_ENABLE);
}

void platform_init(void)
{
    irqInit();
    irqEnable(IRQ_VBLANK);
    REG_IME = 1;
    gfx_init();
}

void platform_wait_vblank(void)
{
    VBlankIntrWait();
}

void platform_read_input(input_t *input)
{
    scanKeys();
    input->pressed = keysDown();
    input->released = keysUp();
    input->held = keysHeld();
    input->repeated = keysDownRepeat();
}

void platform_set_key_repeat(unsigned delay, unsigned rate)
{
    setRepeat((int)delay, (int)rate);
}

void platform_clock_read(datetime_t *now)
{
    u8 bcd[RTC_DATETIME_BYTES];
    rtc_read_datetime(bcd);
    datetime_from_bcd(now, bcd);
}

void platform_clock_write(const datetime_t *now)
{
    u8 bcd[RTC_DATETIME_BYTES];
    datetime_to_bcd(now, bcd);
    rtc_write_datetime(bcd);
}

void platform_settings_load(uint16_t words[SETTINGS_WORDS])
{
    config_flash_read(CONFIG_SETTINGS_OFFSET, words, SETTINGS_WORDS);
}

void platform_settings_store(const uint16_t words[SETTINGS_WORDS])
{
    config_flash_write_block(CONFIG_SETTINGS_OFFSET, words, CONFIG_SETTINGS_BYTES);
}

uint16_t platform_fpga_version(void)
{
    return omega_read_fpga_version();
}
