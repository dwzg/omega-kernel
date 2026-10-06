/**
 * @file settings.h
 * @brief User settings and their on-flash representation.
 *
 * Settings are stored as 16-bit words in the S71 flash (see
 * hal/config_flash.h). The layout is shared with earlier kernels, so a
 * cartridge keeps its settings across an upgrade:
 *
 * | Word | Meaning                       | Values (invalid -> default)        |
 * |------|-------------------------------|------------------------------------|
 * | 0    | Language (legacy)             | always written as 0xE1E1 (English) |
 * | 1    | Reset-to-menu hook            | 0 / 1 (default 0)                  |
 * | 2    | Real-time save hook           | 0 / 1 (default 0)                  |
 * | 3    | Sleep hook                    | 0 / 1 (default 0)                  |
 * | 4    | Cheats                        | 0 / 1 (default 0)                  |
 * | 5-7  | Sleep (or save-state) hotkey  | key index 0-9 (default L, R, SELECT)|
 * | 8-10 | Menu (or load-state) hotkey   | key index 0-9 (default L, R, START) |
 * | 11   | Fast patch engine             | 0 / 1 (default 1)                  |
 * | 12   | Thumbnails (legacy UI)        | 0 / 1 (default 0)                  |
 * | 13   | RTC visible to games          | 0 / 1 (default 1)                  |
 *
 * Words beyond 13 are preserved untouched when settings are saved.
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_SETTINGS_H
#define CORE_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Number of 16-bit words in the settings block. */
#define SETTINGS_WORDS 0x100

/** Value of word 0 that marks English. */
#define SETTINGS_LANGUAGE_ENGLISH 0xE1E1u

/** Word indices in the settings block. */
enum {
    SETTINGS_WORD_LANGUAGE = 0,
    SETTINGS_WORD_RESET_HOOK = 1,
    SETTINGS_WORD_SAVE_STATE_HOOK = 2,
    SETTINGS_WORD_SLEEP_HOOK = 3,
    SETTINGS_WORD_CHEATS = 4,
    SETTINGS_WORD_SLEEP_KEYS = 5, /* 5, 6, 7 */
    SETTINGS_WORD_MENU_KEYS = 8,  /* 8, 9, 10 */
    SETTINGS_WORD_FAST_PATCH = 11,
    SETTINGS_WORD_THUMBNAILS = 12,
    SETTINGS_WORD_GAME_RTC = 13,
    SETTINGS_WORD_LIST_ART = 14 /* new in 2.1; the original kernel used 0..13 */
};

/**
 * Button numbers used in hotkeys. They equal the bit positions in the GBA
 * KEYINPUT register.
 */
typedef enum {
    BUTTON_A = 0,
    BUTTON_B = 1,
    BUTTON_SELECT = 2,
    BUTTON_START = 3,
    BUTTON_RIGHT = 4,
    BUTTON_LEFT = 5,
    BUTTON_UP = 6,
    BUTTON_DOWN = 7,
    BUTTON_R = 8,
    BUTTON_L = 9,
    BUTTON_COUNT = 10
} button_t;

/** Number of buttons in a hotkey combination. */
#define HOTKEY_BUTTONS 3

/** Decoded settings. */
typedef struct {
    bool reset_hook;                    /**< Hotkey returns from a game to the kernel. */
    bool save_state_hook;               /**< Real-time save states (`.rts`). */
    bool sleep_hook;                    /**< Hotkey puts the GBA to sleep. */
    bool cheats;                        /**< Apply selected cheat codes. */
    bool fast_patch;                    /**< Use the patch location database. */
    bool thumbnails;                    /**< Legacy setting, preserved but unused. */
    bool game_rtc;                      /**< Games can see the cartridge clock. */
    bool list_art;                      /**< Box art next to the SD card list. */
    uint8_t sleep_keys[HOTKEY_BUTTONS]; /**< Sleep (or save-state) hotkey. */
    uint8_t menu_keys[HOTKEY_BUTTONS];  /**< Menu (or load-state) hotkey. */
} settings_t;

/** @brief Decode a settings block, replacing invalid values with defaults. */
void settings_decode(settings_t *s, const uint16_t words[SETTINGS_WORDS]);

/**
 * @brief Encode @p s into @p words.
 * Only the words listed in the table above are written; others keep their value.
 */
void settings_encode(const settings_t *s, uint16_t words[SETTINGS_WORDS]);

/** @brief true if any in-game hook (reset, save state, sleep, cheats) is enabled. */
bool settings_any_hook(const settings_t *s);

/**
 * @brief true if save states are the only hook.
 *
 * In this mode a smaller patch is used and the two hotkeys mean "save state"
 * and "load state" instead of "sleep" and "menu".
 */
bool settings_save_state_only(const settings_t *s);

/** @brief KEYINPUT mask (active-low, 10 bits) that the game patch compares against. */
uint16_t settings_hotkey_mask(const uint8_t keys[HOTKEY_BUTTONS]);

/**
 * @brief Move hotkey slot @p slot to the next (@p direction > 0) or previous
 * button that is not used by the other two slots.
 */
void settings_hotkey_step(uint8_t keys[HOTKEY_BUTTONS], unsigned slot, int direction);

/** @brief Display name of a button ("A", "SELECT", "L", ...). */
const char *settings_button_name(uint8_t button);

/** @brief Format a hotkey as "L + R + START". */
void settings_hotkey_format(char *dst, size_t size, const uint8_t keys[HOTKEY_BUTTONS]);

#endif /* CORE_SETTINGS_H */
