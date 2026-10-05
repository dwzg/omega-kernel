/**
 * @file system_screens.c
 * @brief Start-up screens: fatal errors and the FPGA firmware update.
 */
#include <stdio.h>

#include "loader/firmware.h"
#include "ui/app.h"
#include "ui/theme.h"
#include "ui/widgets.h"

/** Show @p text with a hint at the bottom and stop. */
static void __attribute__((noreturn)) halt_with(const char *title, const char *text,
                                                const char *hint)
{
    ui_title_bar(title, NULL);
    ui_clear_content();
    ui_draw_paragraph(16, CONTENT_TOP + 30, GFX_WIDTH - 32, text, COLOR_TEXT, true);
    gfx_text(&FONT_BODY, GFX_WIDTH / 2, CONTENT_BOTTOM - 24, COLOR_TEXT_MUTED, hint, 0,
             ALIGN_CENTER);
    ui_hints("");
    for (;;) {
        platform_wait_vblank();
    }
}

void ui_startup_screen(void)
{
    gfx_fill(0, 0, GFX_WIDTH, GFX_HEIGHT, COLOR_BACKGROUND);
    gfx_text(&FONT_TITLE, GFX_WIDTH / 2, 62, COLOR_TEXT, "Omega Kernel", 0, ALIGN_CENTER);
    gfx_text(&FONT_SMALL, GFX_WIDTH / 2, 82, COLOR_TEXT_MUTED, "Reading the SD card...", 0,
             ALIGN_CENTER);
}

void ui_fatal(const char *title, const char *text)
{
    halt_with(title, text, "Turn the GBA off.");
}

void ui_firmware_update(uint16_t current_version)
{
    char text[160];

    if (!firmware_image_valid()) {
        ui_message("Firmware Update",
                   "The firmware image in this kernel is damaged, so the cartridge can't be "
                   "updated. Please download the kernel again.");
        return;
    }

    snprintf(text, sizeof(text),
             "This cartridge runs firmware %u. Update it to version %u? Keep the GBA on until "
             "the update has finished.",
             current_version, FIRMWARE_BUNDLED_VERSION);
    if (!ui_confirm("Firmware Update", text, "Update")) {
        return;
    }

    firmware_write(ui_progress_begin("Updating Firmware"));
    halt_with("Firmware Updated",
              "The new firmware is active after the GBA has been turned off and on again.",
              "Turn the GBA off.");
}
