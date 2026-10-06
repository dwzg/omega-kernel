/**
 * @file test_library.c
 * @brief Tests for core/save_type, core/recent, core/list_view and core/crc32.
 */
#include <stdio.h>
#include <string.h>

#include "check.h"
#include "core/crc32.h"
#include "core/favorites.h"
#include "core/list_view.h"
#include "core/recent.h"
#include "core/save_type.h"

TEST(save_type_lookup_known_and_unknown)
{
    CHECK_EQ(SAVE_MODE_FLASH_128K, save_type_lookup("BPEE")); /* Pokemon Emerald */
    CHECK_EQ(SAVE_MODE_FLASH_64K, save_type_lookup("AMKE"));  /* Mario Kart */
    CHECK_EQ(SAVE_MODE_SRAM, save_type_lookup("AD2J"));
    CHECK_EQ(SAVE_MODE_DEFAULT, save_type_lookup("ZZZZ"));
    CHECK_EQ(SAVE_MODE_DEFAULT, save_type_lookup("FFFF")); /* the end marker */
    /* Corrected entry: Gauntlet - Dark Legacy uses 512 B EEPROM. */
    CHECK_EQ(SAVE_MODE_EEPROM_512, save_type_lookup("AYGE"));
}

/** Detect in a block of zero words with @p marker at byte offset @p at. */
static save_mode_t detect_with(const char *marker, unsigned at, uint32_t rom_size)
{
    static uint32_t words[64];
    memset(words, 0, sizeof(words));
    memcpy((char *)words + at, marker, strlen(marker));
    return save_type_detect(words, 64, rom_size);
}

TEST(save_type_detect_finds_library_markers)
{
    CHECK_EQ(SAVE_MODE_EEPROM_8K, detect_with("EEPROM_V124", 16, 0x800000));
    CHECK_EQ(SAVE_MODE_EEPROM_8K_BIG, detect_with("EEPROM_V124", 16, 0x2000000));
    CHECK_EQ(SAVE_MODE_SRAM, detect_with("SRAM_V113", 40, 0x400000));
    CHECK_EQ(SAVE_MODE_SRAM, detect_with("SRAM_F_V102", 40, 0x400000));
    CHECK_EQ(SAVE_MODE_FLASH_64K, detect_with("FLASH_V126", 100, 0x400000));
    CHECK_EQ(SAVE_MODE_FLASH512_64K, detect_with("FLASH512_V131", 100, 0x400000));
    CHECK_EQ(SAVE_MODE_FLASH_128K, detect_with("FLASH1M_V103", 200, 0x1000000));
    /* Last possible place: the marker's first three words fit. */
    CHECK_EQ(SAVE_MODE_FLASH_128K, detect_with("FLASH1M_V103", 244, 0x1000000));
}

TEST(save_type_detect_ignores_lookalikes)
{
    CHECK_EQ(SAVE_MODE_DEFAULT, detect_with("", 0, 0x400000));
    CHECK_EQ(SAVE_MODE_DEFAULT, detect_with("EEPROM_V124", 17, 0x400000)); /* not aligned */
    CHECK_EQ(SAVE_MODE_DEFAULT, detect_with("SRAM_X", 8, 0x400000));
    CHECK_EQ(SAVE_MODE_DEFAULT, detect_with("FLASH2M_V1", 8, 0x400000));
    CHECK_EQ(SAVE_MODE_DEFAULT, detect_with("EEPROM_V", 248, 0x400000)); /* cut off */
}

TEST(save_type_known_matches_database)
{
    CHECK(save_type_known("BPEE"));
    CHECK(!save_type_known("ZZZZ"));
    CHECK(!save_type_known("FFFF"));
}

TEST(save_type_resolve_eeprom_depends_on_rom_size)
{
    CHECK_EQ(SAVE_MODE_EEPROM_8K, save_type_resolve(SAVE_CHOICE_EEPROM_8K, "ZZZZ", 0x800000));
    CHECK_EQ(SAVE_MODE_EEPROM_8K, save_type_resolve(SAVE_CHOICE_EEPROM_8K, "ZZZZ", 0x1200000));
    CHECK_EQ(SAVE_MODE_EEPROM_8K_BIG, save_type_resolve(SAVE_CHOICE_EEPROM_8K, "ZZZZ", 0x1200001));
    CHECK_EQ(SAVE_MODE_FLASH_128K, save_type_resolve(SAVE_CHOICE_AUTO, "BPEE", 0x1000000));
    CHECK_EQ(SAVE_MODE_SRAM, save_type_resolve(SAVE_CHOICE_SRAM, "BPEE", 0x1000000));
}

TEST(save_type_sizes_and_names)
{
    CHECK_EQ(0, save_type_file_size(SAVE_MODE_NONE));
    CHECK_EQ(0x8000, save_type_file_size(SAVE_MODE_SRAM));
    CHECK_EQ(0x10000, save_type_file_size(SAVE_MODE_SRAM_64K));
    CHECK_EQ(0x200, save_type_file_size(SAVE_MODE_EEPROM_512));
    CHECK_EQ(0x2000, save_type_file_size(SAVE_MODE_EEPROM_8K_BIG));
    CHECK_EQ(0x20000, save_type_file_size(SAVE_MODE_FLASH_128K));
    CHECK_STR("Flash 128K", save_choice_name(SAVE_CHOICE_FLASH_128K));
    CHECK_STR("Auto", save_choice_name((save_choice_t)99));
    CHECK_STR("EEPROM 8K", save_mode_name(SAVE_MODE_EEPROM_8K_BIG));
}

TEST(save_type_db_is_well_formed)
{
    unsigned n = 0;
    for (const save_type_db_entry_t *e = save_type_db; memcmp(e->game_code, "FFFF", 4) != 0;
         e++, n++) {
        uint8_t m = e->save_mode;
        CHECK(m == 0x00 || m == 0x11 || m == 0x21 || m == 0x22 || m == 0x23 || m == 0x31 ||
              m == 0x32 || m == 0x33);
    }
    CHECK(n > 2000);
}

TEST(favorites_add_find_remove)
{
    static favorites_t f;
    favorites_clear(&f);
    CHECK(favorites_add(&f, "/A.gba"));
    CHECK(favorites_add(&f, "/GBA/B.gba"));
    CHECK(favorites_add(&f, "/A.gba")); /* already there: no duplicate */
    CHECK_EQ(2, f.count);
    CHECK_EQ(1, favorites_find(&f, "/GBA/B.gba"));
    favorites_remove(&f, "/A.gba");
    CHECK_EQ(1, f.count);
    CHECK_STR("/GBA/B.gba", f.entries[0]);
    CHECK_EQ(-1, favorites_find(&f, "/A.gba"));
    favorites_remove(&f, "/missing.gba"); /* no effect */
    CHECK_EQ(1, f.count);
}

TEST(favorites_limits_and_file_lines)
{
    static favorites_t f;
    char path[32];
    favorites_clear(&f);
    CHECK(favorites_append_line(&f, "/One.gba\r\n"));
    CHECK(favorites_append_line(&f, "not a path\n")); /* skipped */
    CHECK(favorites_append_line(&f, "/One.gba\n"));   /* duplicate skipped */
    CHECK_EQ(1, f.count);
    CHECK_STR("/One.gba", f.entries[0]);
    for (unsigned i = 1; i < FAVORITES_MAX; i++) {
        snprintf(path, sizeof(path), "/Game %u.gba", i);
        CHECK(favorites_add(&f, path));
    }
    CHECK(!favorites_add(&f, "/One more.gba"));
    CHECK(!favorites_append_line(&f, "/One more.gba"));
    CHECK_EQ(FAVORITES_MAX, f.count);
}

TEST(recent_touch_moves_to_front)
{
    recent_list_t list;
    recent_clear(&list);
    recent_touch(&list, "/a.gba");
    recent_touch(&list, "/b.gba");
    recent_touch(&list, "/c.gba");
    CHECK_EQ(3, list.count);
    CHECK_STR("/c.gba", list.entries[0]);
    recent_touch(&list, "/a.gba");
    CHECK_EQ(3, list.count);
    CHECK_STR("/a.gba", list.entries[0]);
    CHECK_STR("/c.gba", list.entries[1]);
    CHECK_STR("/b.gba", list.entries[2]);
}

TEST(recent_touch_drops_oldest_when_full)
{
    recent_list_t list;
    char path[32];
    recent_clear(&list);
    for (int i = 0; i < RECENT_MAX + 3; i++) {
        snprintf(path, sizeof(path), "/game%d.gba", i);
        recent_touch(&list, path);
    }
    CHECK_EQ(RECENT_MAX, list.count);
    CHECK_STR("/game12.gba", list.entries[0]);
    CHECK_STR("/game3.gba", list.entries[RECENT_MAX - 1]);
}

TEST(recent_append_line_validates)
{
    recent_list_t list;
    recent_clear(&list);
    CHECK(recent_append_line(&list, "/GBA/Game.gba\r\n"));
    CHECK(!recent_append_line(&list, "garbage"));
    CHECK(!recent_append_line(&list, ""));
    CHECK_EQ(1, list.count);
    CHECK_STR("/GBA/Game.gba", list.entries[0]);
}

static bool odd_rows_only(void *ctx, unsigned index)
{
    (void)ctx;
    return index % 2 == 1;
}

TEST(list_view_scrolls_to_keep_selection_visible)
{
    list_view_t lv;
    list_view_init(&lv, 20, 9, 0);
    CHECK_EQ(0, lv.top);
    CHECK(list_view_move(&lv, 9));
    CHECK_EQ(9, lv.selected);
    CHECK_EQ(1, lv.top);
    CHECK(list_view_move(&lv, 100));
    CHECK_EQ(19, lv.selected);
    CHECK_EQ(11, lv.top);
    CHECK(!list_view_move(&lv, 1)); /* already at the end */
    CHECK(list_view_page(&lv, -1));
    CHECK_EQ(11, lv.selected);
}

TEST(list_view_skips_unselectable_rows)
{
    list_view_t lv;
    list_view_init(&lv, 6, 9, 0);
    list_view_set_selectable(&lv, odd_rows_only, NULL);
    CHECK_EQ(1, lv.selected);
    list_view_move(&lv, 1);
    CHECK_EQ(3, lv.selected);
    list_view_move(&lv, -5);
    CHECK_EQ(1, lv.selected);
}

TEST(list_view_empty_and_restore)
{
    list_view_t lv;
    list_view_init(&lv, 0, 9, 5);
    CHECK_EQ(0, lv.selected);
    CHECK(!list_view_move(&lv, 1));
    list_view_init(&lv, 5, 9, 0);
    list_view_restore(&lv, 50, 40);
    CHECK_EQ(4, lv.selected);
    CHECK_EQ(0, lv.top);
}

TEST(crc32_matches_reference)
{
    CHECK_EQ(0xCBF43926u, crc32("123456789", 9));
    CHECK_EQ(0, crc32("", 0));
}

SUITE(library)
{
    RUN(save_type_lookup_known_and_unknown);
    RUN(favorites_add_find_remove);
    RUN(favorites_limits_and_file_lines);
    RUN(save_type_detect_finds_library_markers);
    RUN(save_type_detect_ignores_lookalikes);
    RUN(save_type_known_matches_database);
    RUN(save_type_resolve_eeprom_depends_on_rom_size);
    RUN(save_type_sizes_and_names);
    RUN(save_type_db_is_well_formed);
    RUN(recent_touch_moves_to_front);
    RUN(recent_touch_drops_oldest_when_full);
    RUN(recent_append_line_validates);
    RUN(list_view_scrolls_to_keep_selection_visible);
    RUN(list_view_skips_unselectable_rows);
    RUN(list_view_empty_and_restore);
    RUN(crc32_matches_reference);
}
