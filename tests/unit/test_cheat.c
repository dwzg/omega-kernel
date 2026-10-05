/**
 * @file test_cheat.c
 * @brief Tests for the `.cht` cheat file parser (core/cheat).
 */
#include <stdlib.h>

#include "check.h"
#include "core/cheat.h"

/** A cht_reader_t over a string in memory. */
typedef struct {
    const char *text;
    size_t pos;
} mem_file_t;

static void mem_rewind(void *ctx)
{
    ((mem_file_t *)ctx)->pos = 0;
}

static bool mem_read_line(void *ctx, char *buf, size_t size)
{
    mem_file_t *f = ctx;
    size_t n = 0;
    if (f->text[f->pos] == '\0') {
        return false;
    }
    while (n + 1 < size && f->text[f->pos] != '\0') {
        char c = f->text[f->pos++];
        buf[n++] = c;
        if (c == '\n') {
            break;
        }
    }
    buf[n] = '\0';
    return true;
}

static cht_reader_t reader_for(mem_file_t *f, const char *text)
{
    f->text = text;
    f->pos = 0;
    return (cht_reader_t){f, mem_rewind, mem_read_line};
}

static const char SAMPLE[] = "[Infinite Health]\r\n"
                             "ON=2002F3A,64;\r\n"
                             "\r\n"
                             "[Money]\r\n"
                             "9999=2001000,0F,27;\r\n"
                             "0=2001000,00,00;\r\n"
                             "\r\n"
                             "// a comment\r\n"
                             "[Walk Through Walls]\r\n"
                             "ON=3001234,01;\r\n"
                             "3005678,FF;\r\n"
                             "\r\n"
                             "[GameInfo]\r\n"
                             "Name=Sample Quest\r\n"
                             "System=GBA\r\n";

TEST(cht_list_entries_reads_sections_and_options)
{
    mem_file_t f;
    cht_reader_t r = reader_for(&f, SAMPLE);
    cht_entry_t entries[32];
    size_t n = cht_list_entries(&r, entries, 32);

    CHECK_EQ(7, n);                                 /* [GameInfo] and what follows are hidden */
    CHECK_STR("InfiniteHealth", entries[0].name);   /* spaces are dropped for matching */
    CHECK_STR("Infinite Health", entries[0].label); /* but kept for display */
    CHECK(entries[0].is_section);
    CHECK_STR("ON", entries[1].name);
    CHECK(!entries[1].is_section);
    CHECK_STR("Money", entries[2].name);
    CHECK_STR("9999", entries[3].name);
    CHECK_STR("0", entries[4].name);
    CHECK_STR("WalkThroughWalls", entries[5].name);
    CHECK_STR("Walk Through Walls", entries[5].label);
    CHECK_STR("9999", entries[3].label);
    CHECK_STR("ON", entries[6].name);
}

TEST(cht_list_entries_respects_limit_and_stop_marker)
{
    mem_file_t f;
    cht_reader_t r = reader_for(&f, "[A]\nx=1,1;\n--\n[B]\ny=2,2;\n");
    cht_entry_t entries[8];
    CHECK_EQ(2, cht_list_entries(&r, entries, 8));
    r = reader_for(&f, SAMPLE);
    CHECK_EQ(3, cht_list_entries(&r, entries, 3));
}

TEST(cht_list_entries_skips_block_comments)
{
    mem_file_t f;
    cht_reader_t r = reader_for(&f, "/* start\n[Hidden]\nx=1,1;\n end */\n[Shown]\ny=2,2;\n");
    cht_entry_t entries[8];
    size_t n = cht_list_entries(&r, entries, 8);
    CHECK_EQ(2, n);
    CHECK_STR("Shown", entries[0].name);
}

TEST(cht_read_game_name_from_game_info)
{
    mem_file_t f;
    cht_reader_t r = reader_for(&f, SAMPLE);
    char name[CHT_NAME_LEN];
    cht_read_game_name(&r, name, sizeof(name));
    CHECK_STR("Sample Quest", name);
    r = reader_for(&f, "[A]\nx=1,1;\n");
    cht_read_game_name(&r, name, sizeof(name));
    CHECK_STR("", name);
}

TEST(cht_read_value_joins_continuation_lines)
{
    mem_file_t f;
    cht_reader_t r = reader_for(&f, SAMPLE);
    static cht_workspace_t ws;
    size_t len = cht_read_value(&r, "WalkThroughWalls", "ON", &ws);
    CHECK_EQ(strlen("3001234,01;3005678,FF;"), len);
    CHECK(memcmp(ws.value, "3001234,01;3005678,FF;", len) == 0);
    CHECK_EQ(0, cht_read_value(&r, "Money", "missing", &ws));
}

TEST(cht_decode_value_expands_byte_runs)
{
    static const char v[] = "2001000,0F,27;3001234,01";
    cheat_code_t codes[8];
    size_t n = 0;
    cht_decode_value(v, strlen(v), codes, 8, &n);
    CHECK_EQ(3, n);
    CHECK_EQ(0x2001000, codes[0].address);
    CHECK_EQ(0x0F, codes[0].value);
    CHECK_EQ(0x2001001, codes[1].address);
    CHECK_EQ(0x27, codes[1].value);
    CHECK_EQ(0x3001234, codes[2].address); /* no ';' at the end */
    CHECK_EQ(0x01, codes[2].value);
}

TEST(cht_decode_value_no_bogus_code_after_semicolon)
{
    static const char v[] = "2002F3A,64;";
    cheat_code_t codes[8];
    size_t n = 0;
    cht_decode_value(v, strlen(v), codes, 8, &n);
    CHECK_EQ(1, n);
}

TEST(cht_decode_value_respects_capacity)
{
    static const char v[] = "2000000,01,02,03,04,05;";
    cheat_code_t codes[3];
    size_t n = 0;
    cht_decode_value(v, strlen(v), codes, 3, &n);
    CHECK_EQ(3, n);
    CHECK_EQ(0x2000002, codes[2].address);
}

TEST(cht_toggle_option_is_exclusive_within_section)
{
    mem_file_t f;
    cht_reader_t r = reader_for(&f, SAMPLE);
    cht_entry_t entries[32];
    size_t n = cht_list_entries(&r, entries, 32);
    cht_toggle_option(entries, n, 3); /* Money: 9999 */
    cht_toggle_option(entries, n, 4); /* Money: 0 replaces 9999 */
    cht_toggle_option(entries, n, 1); /* Infinite Health: ON */
    cht_toggle_option(entries, n, 0); /* headings are ignored */
    CHECK(!entries[3].selected);
    CHECK(entries[4].selected);
    CHECK(entries[1].selected);
    CHECK(!entries[0].selected);
    CHECK_EQ(2, cht_selected_count(entries, n));
    cht_toggle_option(entries, n, 4); /* off again */
    CHECK_EQ(1, cht_selected_count(entries, n));
}

TEST(cht_collect_codes_for_selected_options)
{
    mem_file_t f;
    cht_reader_t r = reader_for(&f, SAMPLE);
    cht_entry_t entries[32];
    static cht_workspace_t ws;
    cheat_code_t codes[16];
    size_t n = cht_list_entries(&r, entries, 32);
    cht_toggle_option(entries, n, 3); /* Money 9999: 2 codes */
    cht_toggle_option(entries, n, 6); /* Walk through walls: 2 codes */
    size_t got = cht_collect_codes(&r, entries, n, &ws, codes, 16);
    CHECK_EQ(4, got);
    CHECK_EQ(0x2001000, codes[0].address);
    CHECK_EQ(0x0F, codes[0].value);
    CHECK_EQ(0x2001001, codes[1].address);
    CHECK_EQ(0x27, codes[1].value);
    CHECK_EQ(0x3001234, codes[2].address);
    CHECK_EQ(0x3005678, codes[3].address);
    CHECK_EQ(0xFF, codes[3].value);
}

TEST(cht_library_paths_buckets_by_number)
{
    char folder[32];
    char file[16];
    /* "0042" stored first digit in the low byte. */
    uint32_t id = '0' | ('0' << 8) | ('4' << 16) | ((uint32_t)'2' << 24);
    cht_library_paths(id, folder, sizeof(folder), file, sizeof(file));
    CHECK_STR("/CHEAT/Eng/0000", folder);
    CHECK_STR("0042.cht", file);
    id = '2' | ('9' << 8) | ('9' << 16) | ((uint32_t)'9' << 24);
    cht_library_paths(id, folder, sizeof(folder), file, sizeof(file));
    CHECK_STR("/CHEAT/Eng/2800", folder);
    CHECK_STR("2999.cht", file);
    id = '1' | ('2' << 8) | ('3' << 16) | ((uint32_t)'4' << 24);
    cht_library_paths(id, folder, sizeof(folder), file, sizeof(file));
    CHECK_STR("/CHEAT/Eng/1200", folder);
}

SUITE(cheat)
{
    RUN(cht_list_entries_reads_sections_and_options);
    RUN(cht_list_entries_respects_limit_and_stop_marker);
    RUN(cht_list_entries_skips_block_comments);
    RUN(cht_read_game_name_from_game_info);
    RUN(cht_read_value_joins_continuation_lines);
    RUN(cht_decode_value_expands_byte_runs);
    RUN(cht_decode_value_no_bogus_code_after_semicolon);
    RUN(cht_decode_value_respects_capacity);
    RUN(cht_toggle_option_is_exclusive_within_section);
    RUN(cht_collect_codes_for_selected_options);
    RUN(cht_library_paths_buckets_by_number);
}
