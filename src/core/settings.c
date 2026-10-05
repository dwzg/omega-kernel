/**
 * @file settings.c
 * @brief Settings encoding and hotkey helpers. See settings.h.
 */
#include "core/settings.h"

#include <stdio.h>

static const uint8_t DEFAULT_SLEEP_KEYS[HOTKEY_BUTTONS] = {BUTTON_L, BUTTON_R, BUTTON_SELECT};
static const uint8_t DEFAULT_MENU_KEYS[HOTKEY_BUTTONS] = {BUTTON_L, BUTTON_R, BUTTON_START};

static bool decode_bool(uint16_t word, bool fallback)
{
    if (word == 0) {
        return false;
    }
    if (word == 1) {
        return true;
    }
    return fallback;
}

static void decode_keys(uint8_t keys[HOTKEY_BUTTONS], const uint16_t *words,
                        const uint8_t defaults[HOTKEY_BUTTONS])
{
    for (unsigned i = 0; i < HOTKEY_BUTTONS; i++) {
        keys[i] = (words[i] < BUTTON_COUNT) ? (uint8_t)words[i] : defaults[i];
    }
}

void settings_decode(settings_t *s, const uint16_t words[SETTINGS_WORDS])
{
    s->reset_hook = decode_bool(words[SETTINGS_WORD_RESET_HOOK], false);
    s->save_state_hook = decode_bool(words[SETTINGS_WORD_SAVE_STATE_HOOK], false);
    s->sleep_hook = decode_bool(words[SETTINGS_WORD_SLEEP_HOOK], false);
    s->cheats = decode_bool(words[SETTINGS_WORD_CHEATS], false);
    s->fast_patch = decode_bool(words[SETTINGS_WORD_FAST_PATCH], true);
    s->thumbnails = decode_bool(words[SETTINGS_WORD_THUMBNAILS], false);
    s->game_rtc = decode_bool(words[SETTINGS_WORD_GAME_RTC], true);
    decode_keys(s->sleep_keys, &words[SETTINGS_WORD_SLEEP_KEYS], DEFAULT_SLEEP_KEYS);
    decode_keys(s->menu_keys, &words[SETTINGS_WORD_MENU_KEYS], DEFAULT_MENU_KEYS);
}

void settings_encode(const settings_t *s, uint16_t words[SETTINGS_WORDS])
{
    words[SETTINGS_WORD_LANGUAGE] = SETTINGS_LANGUAGE_ENGLISH;
    words[SETTINGS_WORD_RESET_HOOK] = s->reset_hook;
    words[SETTINGS_WORD_SAVE_STATE_HOOK] = s->save_state_hook;
    words[SETTINGS_WORD_SLEEP_HOOK] = s->sleep_hook;
    words[SETTINGS_WORD_CHEATS] = s->cheats;
    for (unsigned i = 0; i < HOTKEY_BUTTONS; i++) {
        words[SETTINGS_WORD_SLEEP_KEYS + i] = s->sleep_keys[i];
        words[SETTINGS_WORD_MENU_KEYS + i] = s->menu_keys[i];
    }
    words[SETTINGS_WORD_FAST_PATCH] = s->fast_patch;
    words[SETTINGS_WORD_THUMBNAILS] = s->thumbnails;
    words[SETTINGS_WORD_GAME_RTC] = s->game_rtc;
}

bool settings_any_hook(const settings_t *s)
{
    return s->reset_hook || s->save_state_hook || s->sleep_hook || s->cheats;
}

bool settings_save_state_only(const settings_t *s)
{
    return s->save_state_hook && !s->reset_hook && !s->sleep_hook && !s->cheats;
}

uint16_t settings_hotkey_mask(const uint8_t keys[HOTKEY_BUTTONS])
{
    uint16_t pressed = 0;
    for (unsigned i = 0; i < HOTKEY_BUTTONS; i++) {
        pressed |= (uint16_t)(1u << keys[i]);
    }
    return (uint16_t)(~pressed & 0x3FF);
}

void settings_hotkey_step(uint8_t keys[HOTKEY_BUTTONS], unsigned slot, int direction)
{
    uint8_t k = keys[slot];
    do {
        if (direction > 0) {
            k = (k >= BUTTON_COUNT - 1) ? 0 : (uint8_t)(k + 1);
        } else {
            k = (k == 0 || k >= BUTTON_COUNT) ? BUTTON_COUNT - 1 : (uint8_t)(k - 1);
        }
    } while (k == keys[(slot + 1) % HOTKEY_BUTTONS] || k == keys[(slot + 2) % HOTKEY_BUTTONS]);
    keys[slot] = k;
}

const char *settings_button_name(uint8_t button)
{
    static const char *const names[BUTTON_COUNT] = {
        "A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L",
    };
    return button < BUTTON_COUNT ? names[button] : "?";
}

void settings_hotkey_format(char *dst, size_t size, const uint8_t keys[HOTKEY_BUTTONS])
{
    snprintf(dst, size, "%s + %s + %s", settings_button_name(keys[0]),
             settings_button_name(keys[1]), settings_button_name(keys[2]));
}
