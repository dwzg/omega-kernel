/**
 * @file test_text.c
 * @brief Tests for core/text, core/path and core/game_file.
 */
#include "check.h"
#include "core/game_file.h"
#include "core/path.h"
#include "core/text.h"
#include "core/utf8.h"

TEST(fold_ignores_case_and_accents)
{
    CHECK_EQ('e', text_fold('E'));
    CHECK_EQ('e', text_fold(0xC9));      /* É */
    CHECK_EQ('s', text_fold(0xDF));      /* ß */
    CHECK_EQ('z', text_fold(0x17D));     /* Ž */
    CHECK_EQ('l', text_fold(0x141));     /* Ł */
    CHECK_EQ(0xD7, text_fold(0xD7));     /* × is not a letter */
    CHECK_EQ(0x30A2, text_fold(0x30A2)); /* katakana stays */
    CHECK_EQ('1', text_fold('1'));
}

static int sign(int v)
{
    return (v > 0) - (v < 0);
}

TEST(names_sort_naturally)
{
    CHECK_EQ(-1, sign(text_compare_names("Mega Man 2", "Mega Man 10")));
    CHECK_EQ(1, sign(text_compare_names("Mega Man 10", "Mega Man 2")));
    CHECK_EQ(-1, sign(text_compare_names("Disc 002", "Disc 10")));
    CHECK_EQ(-1, sign(text_compare_names("a9b", "a10a")));
    CHECK_EQ(-1, sign(text_compare_names("Zelda", "zeldb")));
    CHECK_EQ(-1, sign(text_compare_names("Egypt", "\xC3\x89gypte"))); /* Égypte after Egypt */
    CHECK_EQ(-1, sign(text_compare_names("\xC3\x89gypte", "Zelda")));
    CHECK_EQ(-1, sign(text_compare_names("Pok\xC3\xA9mon", "Pokemon Z")));
    CHECK_EQ(-1, sign(text_compare_names("Game", "Game 2")));
    CHECK_EQ(-1, sign(text_compare_names("Zelda", "\xE3\x82\xA2"))); /* kana after Latin */
    /* Equal when folded: a fixed order, and a name equals only itself. */
    CHECK_EQ(-sign(text_compare_names("abc", "ABC")), sign(text_compare_names("ABC", "abc")));
    CHECK(text_compare_names("abc", "ABC") != 0);
    CHECK(text_compare_names("Game 01", "Game 1") != 0);
    CHECK_EQ(0, text_compare_names("Same", "Same"));
}

TEST(text_copy_fits)
{
    char buf[8];
    CHECK(text_copy(buf, sizeof(buf), "abc"));
    CHECK_STR("abc", buf);
}

TEST(text_copy_truncates)
{
    char buf[4];
    CHECK(!text_copy(buf, sizeof(buf), "abcdef"));
    CHECK_STR("abc", buf);
    CHECK(!text_copy(buf, 0, "x"));
}

TEST(text_trim_right_strips_line_endings)
{
    char s[] = "/GAMES/Test.gba \t\r\n";
    text_trim_right(s);
    CHECK_STR("/GAMES/Test.gba", s);
    char empty[] = " \r\n";
    text_trim_right(empty);
    CHECK_STR("", empty);
}

TEST(text_ends_with_ci_ignores_case)
{
    CHECK(text_ends_with_ci("Game.GBA", ".gba"));
    CHECK(!text_ends_with_ci("gba", ".gba"));
    CHECK(text_ends_with_ci("x", ""));
}

TEST(text_parse_hex_reads_both_cases)
{
    CHECK_EQ(0x0203A0F0u, text_parse_hex("0203a0F0"));
    CHECK_EQ(0, text_parse_hex(""));
    CHECK_EQ(0, text_parse_hex("123456789")); /* longer than 32 bits */
}

TEST(text_format_size_picks_unit)
{
    char buf[16];
    CHECK_STR("512 B", text_format_size(buf, sizeof(buf), 512));
    CHECK_STR("64 KB", text_format_size(buf, sizeof(buf), 0x10000));
    CHECK_STR("8 MB", text_format_size(buf, sizeof(buf), 0x800000));
    CHECK_STR("31 MB", text_format_size(buf, sizeof(buf), 0x1FFFFFF));
}

TEST(path_join_handles_root)
{
    char buf[PATH_MAX_LEN];
    CHECK(path_join(buf, sizeof(buf), "/", "Game.gba"));
    CHECK_STR("/Game.gba", buf);
    CHECK(path_join(buf, sizeof(buf), "/GBA/RPG", "Game.gba"));
    CHECK_STR("/GBA/RPG/Game.gba", buf);
}

TEST(path_join_reports_overflow)
{
    char buf[10];
    CHECK(!path_join(buf, sizeof(buf), "/folder", "name.gba"));
}

TEST(path_to_parent_stops_at_root)
{
    char p[PATH_MAX_LEN] = "/a/b";
    path_to_parent(p);
    CHECK_STR("/a", p);
    path_to_parent(p);
    CHECK_STR("/", p);
    path_to_parent(p);
    CHECK_STR("/", p);
}

TEST(path_basename_and_split)
{
    char dir[PATH_MAX_LEN];
    char name[64];
    CHECK_STR("c.gba", path_basename("/a/b/c.gba"));
    CHECK_STR("plain", path_basename("plain"));
    CHECK(path_split("/a/b/c.gba", dir, sizeof(dir), name, sizeof(name)));
    CHECK_STR("/a/b", dir);
    CHECK_STR("c.gba", name);
    CHECK(path_split("/c.gba", dir, sizeof(dir), name, sizeof(name)));
    CHECK_STR("/", dir);
    CHECK(!path_split("nofolder", dir, sizeof(dir), name, sizeof(name)));
}

TEST(game_file_is_gba_matches_extension_only)
{
    CHECK(game_file_is_gba("Metroid.gba"));
    CHECK(game_file_is_gba("METROID.GBA"));
    CHECK(!game_file_is_gba("Metroid.gb"));
    CHECK(!game_file_is_gba("Metroid.gbc"));
    CHECK(!game_file_is_gba("Metroid.nes"));
    CHECK(!game_file_is_gba("gba"));
}

TEST(game_file_companion_replaces_extension)
{
    char buf[64];
    CHECK(game_file_companion(buf, sizeof(buf), "Zelda.gba", "sav"));
    CHECK_STR("Zelda.sav", buf);
    CHECK(game_file_companion(buf, sizeof(buf), "My.Game.gba", "pat"));
    CHECK_STR("My.Game.pat", buf);
}

TEST(utf8_decodes_common_characters)
{
    const char *p = "A\xC3\xA9\xE3\x83\x9D\xF0\x9F\x8E\xAE";
    CHECK_EQ('A', utf8_next(&p));
    CHECK_EQ(0xE9, utf8_next(&p));    /* é */
    CHECK_EQ(0x30DD, utf8_next(&p));  /* ポ */
    CHECK_EQ(0x1F3AE, utf8_next(&p)); /* 🎮 */
    CHECK_EQ(0, utf8_next(&p));
    CHECK_EQ(0, utf8_next(&p)); /* stays at the end */
}

TEST(utf8_rejects_invalid_bytes)
{
    const char *p = "\x80x\xC3(\xE3\x83\xC0\x80\xED\xA0\x80";
    CHECK_EQ(UTF8_INVALID, utf8_next(&p)); /* stray continuation */
    CHECK_EQ('x', utf8_next(&p));
    CHECK_EQ(UTF8_INVALID, utf8_next(&p)); /* lead byte without continuation */
    CHECK_EQ('(', utf8_next(&p));
    CHECK_EQ(UTF8_INVALID, utf8_next(&p)); /* truncated 3-byte sequence... */
    CHECK_EQ(UTF8_INVALID, utf8_next(&p)); /* ...leaves a stray continuation */
    CHECK_EQ(UTF8_INVALID, utf8_next(&p)); /* overlong encoding (one character) */
    CHECK_EQ(UTF8_INVALID, utf8_next(&p)); /* surrogate */
    CHECK_EQ(0, utf8_next(&p));
}

TEST(text_copy_keeps_characters_whole)
{
    char buf[4];
    CHECK(!text_copy(buf, sizeof(buf), "ab\xC3\xA9")); /* "abé" needs 5 bytes */
    CHECK_STR("ab", buf);
    CHECK_EQ(2, utf8_prefix("ab\xC3\xA9", 3));
    CHECK_EQ(4, utf8_prefix("ab\xC3\xA9", 9));
}

SUITE(text)
{
    RUN(fold_ignores_case_and_accents);
    RUN(names_sort_naturally);
    RUN(text_copy_fits);
    RUN(text_copy_truncates);
    RUN(text_trim_right_strips_line_endings);
    RUN(text_ends_with_ci_ignores_case);
    RUN(text_parse_hex_reads_both_cases);
    RUN(text_format_size_picks_unit);
    RUN(path_join_handles_root);
    RUN(path_join_reports_overflow);
    RUN(path_to_parent_stops_at_root);
    RUN(path_basename_and_split);
    RUN(game_file_is_gba_matches_extension_only);
    RUN(game_file_companion_replaces_extension);
    RUN(utf8_decodes_common_characters);
    RUN(utf8_rejects_invalid_bytes);
    RUN(text_copy_keeps_characters_whole);
}
