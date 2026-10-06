/**
 * @file test_settings.c
 * @brief Tests for core/settings and core/datetime.
 */
#include "check.h"
#include "core/datetime.h"
#include "core/settings.h"

TEST(settings_decode_erased_flash_gives_defaults)
{
    uint16_t words[SETTINGS_WORDS];
    settings_t s;
    memset(words, 0xFF, sizeof(words)); /* erased flash */
    settings_decode(&s, words);
    CHECK(!s.reset_hook);
    CHECK(!s.save_state_hook);
    CHECK(!s.sleep_hook);
    CHECK(!s.cheats);
    CHECK(s.fast_patch);
    CHECK(s.game_rtc);
    CHECK_EQ(BUTTON_L, s.sleep_keys[0]);
    CHECK_EQ(BUTTON_R, s.sleep_keys[1]);
    CHECK_EQ(BUTTON_SELECT, s.sleep_keys[2]);
    CHECK_EQ(BUTTON_START, s.menu_keys[2]);
}

TEST(settings_round_trip_keeps_unknown_words)
{
    uint16_t words[SETTINGS_WORDS];
    settings_t s;
    for (unsigned i = 0; i < SETTINGS_WORDS; i++) {
        words[i] = (uint16_t)(0x1000 + i);
    }
    settings_decode(&s, words);
    s.reset_hook = true;
    s.cheats = true;
    s.fast_patch = false;
    s.menu_keys[2] = BUTTON_A;
    settings_encode(&s, words);

    CHECK_EQ(SETTINGS_LANGUAGE_ENGLISH, words[SETTINGS_WORD_LANGUAGE]);
    CHECK_EQ(1, words[SETTINGS_WORD_RESET_HOOK]);
    CHECK_EQ(0, words[SETTINGS_WORD_SAVE_STATE_HOOK]);
    CHECK_EQ(1, words[SETTINGS_WORD_CHEATS]);
    CHECK_EQ(0, words[SETTINGS_WORD_FAST_PATCH]);
    CHECK_EQ(BUTTON_A, words[SETTINGS_WORD_MENU_KEYS + 2]);
    for (unsigned i = SETTINGS_WORD_LIST_ART + 1; i < SETTINGS_WORDS; i++) {
        CHECK_EQ(0x1000 + i, words[i]);
    }

    settings_t again;
    settings_decode(&again, words);
    CHECK_MEM(&s, &again, sizeof(s));
}

TEST(settings_hook_queries)
{
    settings_t s = {0};
    CHECK(!settings_any_hook(&s));
    CHECK(!settings_save_state_only(&s));
    s.save_state_hook = true;
    CHECK(settings_any_hook(&s));
    CHECK(settings_save_state_only(&s));
    s.sleep_hook = true;
    CHECK(!settings_save_state_only(&s));
}

TEST(settings_hotkey_mask_is_active_low)
{
    const uint8_t keys[HOTKEY_BUTTONS] = {BUTTON_L, BUTTON_R, BUTTON_SELECT};
    /* KEYINPUT reads 0 for pressed buttons: L (bit 9), R (8), SELECT (2). */
    CHECK_EQ(0x3FF & ~((1u << 9) | (1u << 8) | (1u << 2)), settings_hotkey_mask(keys));
}

TEST(settings_hotkey_step_skips_used_buttons)
{
    uint8_t keys[HOTKEY_BUTTONS] = {BUTTON_L, BUTTON_R, BUTTON_SELECT};
    settings_hotkey_step(keys, 2, 1); /* SELECT -> START */
    CHECK_EQ(BUTTON_START, keys[2]);
    settings_hotkey_step(keys, 0, 1); /* L wraps to A */
    CHECK_EQ(BUTTON_A, keys[0]);
    settings_hotkey_step(keys, 0, -1); /* A -> L */
    CHECK_EQ(BUTTON_L, keys[0]);
    settings_hotkey_step(keys, 1, 1); /* R -> L is taken -> A */
    CHECK_EQ(BUTTON_A, keys[1]);
}

TEST(settings_hotkey_format_names_buttons)
{
    char buf[32];
    const uint8_t keys[HOTKEY_BUTTONS] = {BUTTON_L, BUTTON_R, BUTTON_START};
    settings_hotkey_format(buf, sizeof(buf), keys);
    CHECK_STR("L + R + START", buf);
    CHECK_STR("?", settings_button_name(42));
}

TEST(datetime_bcd_round_trip)
{
    const uint8_t bcd[DATETIME_FIELD_COUNT] = {0x24, 0x02, 0x29, 0x04, 0x23, 0x59, 0x58};
    datetime_t dt;
    uint8_t out[DATETIME_FIELD_COUNT];
    datetime_from_bcd(&dt, bcd);
    CHECK_EQ(24, dt.year);
    CHECK_EQ(2, dt.month);
    CHECK_EQ(29, dt.day);
    CHECK_EQ(23, dt.hour);
    CHECK_EQ(59, dt.minute);
    CHECK_EQ(58, dt.second);
    datetime_to_bcd(&dt, out);
    CHECK_MEM(bcd, out, sizeof(out));
}

TEST(datetime_from_bcd_repairs_garbage)
{
    const uint8_t bcd[DATETIME_FIELD_COUNT] = {0xFF, 0x00, 0x00, 0x07, 0x3F, 0x7F, 0x7F};
    datetime_t dt;
    datetime_from_bcd(&dt, bcd);
    CHECK_EQ(0, dt.year);
    CHECK_EQ(1, dt.month);
    CHECK_EQ(1, dt.day);
    CHECK(dt.weekday <= 6);
    CHECK_EQ(0, dt.hour);
    CHECK_EQ(0, dt.minute);
    CHECK_EQ(0, dt.second);
}

TEST(datetime_pm_flag_is_ignored)
{
    const uint8_t bcd[3] = {0x40 | 0x15, 0x30, 0x00}; /* bit 6 = PM flag */
    datetime_t dt = {0};
    datetime_time_from_bcd(&dt, bcd);
    CHECK_EQ(15, dt.hour);
    CHECK_EQ(30, dt.minute);
}

TEST(datetime_days_in_month)
{
    CHECK_EQ(29, days_in_month(2, 24));
    CHECK_EQ(28, days_in_month(2, 25));
    CHECK_EQ(29, days_in_month(2, 0)); /* 2000 was a leap year */
    CHECK_EQ(30, days_in_month(11, 25));
    CHECK_EQ(31, days_in_month(12, 25));
}

TEST(datetime_weekday_known_dates)
{
    datetime_t dt = {.year = 0, .month = 1, .day = 1};
    CHECK_EQ(6, datetime_weekday(&dt)); /* 2000-01-01 was a Saturday */
    dt = (datetime_t){.year = 24, .month = 2, .day = 29};
    CHECK_EQ(4, datetime_weekday(&dt)); /* Thursday */
    dt = (datetime_t){.year = 26, .month = 10, .day = 5};
    CHECK_EQ(1, datetime_weekday(&dt)); /* Monday */
    CHECK_STR("Mon", datetime_weekday_name(1));
    CHECK_STR("Sun", datetime_weekday_name(9));
}

TEST(datetime_to_fat_packs_fields)
{
    datetime_t dt = {.year = 24, .month = 12, .day = 31, .hour = 23, .minute = 59, .second = 59};
    uint32_t fat = datetime_to_fat(&dt);
    CHECK_EQ(44, fat >> 25); /* 2024 - 1980 */
    CHECK_EQ(12, (fat >> 21) & 0xF);
    CHECK_EQ(31, (fat >> 16) & 0x1F);
    CHECK_EQ(23, (fat >> 11) & 0x1F);
    CHECK_EQ(59, (fat >> 5) & 0x3F);
    CHECK_EQ(29, fat & 0x1F);
}

SUITE(settings)
{
    RUN(settings_decode_erased_flash_gives_defaults);
    RUN(settings_round_trip_keeps_unknown_words);
    RUN(settings_hook_queries);
    RUN(settings_hotkey_mask_is_active_low);
    RUN(settings_hotkey_step_skips_used_buttons);
    RUN(settings_hotkey_format_names_buttons);
    RUN(datetime_bcd_round_trip);
    RUN(datetime_from_bcd_repairs_garbage);
    RUN(datetime_pm_flag_is_ignored);
    RUN(datetime_days_in_month);
    RUN(datetime_weekday_known_dates);
    RUN(datetime_to_fat_packs_fields);
}
