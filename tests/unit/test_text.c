/**
 * @file test_text.c
 * @brief Tests for core/text, core/path and core/game_file.
 */
#include "check.h"
#include "core/game_file.h"
#include "core/path.h"
#include "core/text.h"

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

SUITE(text)
{
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
}
