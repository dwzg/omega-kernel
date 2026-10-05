/**
 * @file main.c
 * @brief Kernel entry point: hardware set-up, start-up checks, then the UI.
 *
 * Start-up sequence (see docs/boot-process.md):
 *  1. Initialise interrupts and the display, disable the SD interface and
 *     enable the cartridge clock.
 *  2. Offer an FPGA firmware update if the cartridge runs an older version.
 *  3. Show a plain start-up screen and mount the SD card.
 *  4. Load the settings and the NOR game table.
 *  5. Run the interface (never returns; starting a game resets the GBA).
 */
#include "hal/omega.h"
#include "loader/firmware.h"
#include "loader/nor_games.h"
#include "loader/sd_paths.h"
#include "platform/attributes.h"
#include "platform/platform.h"
#include "ui/app.h"

/** Interface state; lives as long as the kernel runs. */
static app_t s_app PLATFORM_EWRAM;

int main(void)
{
    platform_init();
    omega_set_sd_mode(OMEGA_SD_OFF);
    omega_set_rtc_enabled(1);

    s_app.fpga_version = platform_fpga_version();
    if (firmware_update_available(s_app.fpga_version)) {
        ui_firmware_update(s_app.fpga_version);
    }

    ui_startup_screen();
    if (!sd_mount()) {
        ui_fatal("No SD Card",
                 "The SD card can't be read. Insert a card formatted as FAT32 or exFAT, then "
                 "turn the GBA off and on again.");
    }

    app_load_settings(&s_app);
    nor_games_init();
    ui_run(&s_app);
}
