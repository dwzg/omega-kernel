/**
 * @file platform.h
 * @brief The services the user interface needs from the machine.
 *
 * Implemented for the GBA in src/platform/gba/ and by the host simulator in
 * tests/sim/, which lets the UI run (and be screenshot-tested) on a PC.
 */
#ifndef PLATFORM_PLATFORM_H
#define PLATFORM_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "core/datetime.h"
#include "core/settings.h"

/** @name Button masks (bit positions as in the GBA KEYINPUT register) */
/**@{*/
#define BTN_A (1u << BUTTON_A)
#define BTN_B (1u << BUTTON_B)
#define BTN_SELECT (1u << BUTTON_SELECT)
#define BTN_START (1u << BUTTON_START)
#define BTN_RIGHT (1u << BUTTON_RIGHT)
#define BTN_LEFT (1u << BUTTON_LEFT)
#define BTN_UP (1u << BUTTON_UP)
#define BTN_DOWN (1u << BUTTON_DOWN)
#define BTN_R (1u << BUTTON_R)
#define BTN_L (1u << BUTTON_L)
/**@}*/

/** Button state for one frame. */
typedef struct {
    uint16_t pressed;  /**< Went down this frame. */
    uint16_t released; /**< Went up this frame. */
    uint16_t held;     /**< Currently down. */
    uint16_t repeated; /**< Pressed, or held long enough to auto-repeat. */
} input_t;

/** @brief Set up interrupts and the display. */
void platform_init(void);

/** @brief Wait for the next vertical blank (60 Hz). */
void platform_wait_vblank(void);

/** @brief Sample the buttons (call once per frame). */
void platform_read_input(input_t *input);

/** @brief Auto-repeat timing in frames: first repeat after @p delay, then every @p rate. */
void platform_set_key_repeat(unsigned delay, unsigned rate);

/** @brief Read the cartridge clock. */
void platform_clock_read(datetime_t *now);

/** @brief Set the cartridge clock. */
void platform_clock_write(const datetime_t *now);

/** @brief Read the raw settings block. */
void platform_settings_load(uint16_t words[SETTINGS_WORDS]);

/** @brief Write the raw settings block (erases and programs flash). */
void platform_settings_store(const uint16_t words[SETTINGS_WORDS]);

/** @brief FPGA firmware version of the cartridge. */
uint16_t platform_fpga_version(void);

#endif /* PLATFORM_PLATFORM_H */
