/**
 * @file test_patch.c
 * @brief Tests for the game patch engine (patch/patch.c) with fake payloads
 * and an in-memory PSRAM.
 */
#include <stdlib.h>

#include "check.h"
#include "patch/irq_patch_db.h"
#include "patch/patch.h"
#include "patch/payloads.h"

#define ROM_SIZE 0x100000u
#define WORK_SIZE 0x5400u

static uint8_t s_psram[ROM_SIZE + 0x40000];
static uint8_t s_rom[ROM_SIZE];
static uint8_t s_work[WORK_SIZE];
static uint8_t s_payload_code[0x400];
static unsigned s_writes;

static void fake_write_psram(uint32_t rom_offset, const void *data, uint32_t size)
{
    if (rom_offset + size <= sizeof(s_psram)) {
        memcpy(s_psram + rom_offset, data, size);
        s_writes++;
    }
}

static const patch_platform_t PLATFORM = {fake_write_psram, s_work, WORK_SIZE};

/* One payload layout reused for every payload kind: field words at fixed
 * offsets, the cheat table after 0x40 bytes. */
static payload_t fake_payload(void)
{
    return (payload_t){
        .code = s_payload_code,
        .size = 0x40,
        .return_address = 0x00,
        .key_a = 0x08,
        .key_b = 0x0C,
        .save_state_switch = 0x10,
        .cheat_count = 0x14,
        .cheat_table = 0x40,
        .size_without_cheats = 0x40,
        .modify_address = PAYLOAD_NO_FIELD,
    };
}

static patch_payloads_t s_payloads;

static uint32_t word_at(const uint8_t *buf, uint32_t offset)
{
    uint32_t w;
    memcpy(&w, buf + offset, 4);
    return w;
}

static void put_word(uint8_t *buf, uint32_t offset, uint32_t value)
{
    memcpy(buf + offset, &value, 4);
}

/** A ROM of pseudo-random data up to @p data_end, then 0xFF padding. */
static void make_rom(uint32_t data_end)
{
    srand(1234);
    for (uint32_t i = 0; i < ROM_SIZE; i++) {
        s_rom[i] = i < data_end ? (uint8_t)(rand() % 0xFE) : 0xFF;
    }
    put_word(s_rom, 0, 0xEA00002E); /* b 0x080000C0, as in real headers */
    memcpy(s_rom + 0xAC, "ZTST", 4);
    memset(s_psram, 0, sizeof(s_psram));
    memset(s_payload_code, 0xA5, sizeof(s_payload_code));
    s_payloads.sleep = s_payloads.full = s_payloads.save_state_only = fake_payload();
    s_writes = 0;
}

static void init(patch_context_t *ctx, const settings_t *s, const cheat_code_t *cheats, size_t n)
{
    patch_init(ctx, &PLATFORM, &s_payloads, "ZTST", s, cheats, n);
}

static settings_t default_settings(void)
{
    uint16_t words[SETTINGS_WORDS];
    settings_t s;
    memset(words, 0xFF, sizeof(words));
    settings_decode(&s, words);
    return s;
}

TEST(trim_size_follows_last_data_byte)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    s.reset_hook = true;
    make_rom(0xFFF00);
    init(&ctx, &s, NULL, 0);
    patch_find_trim_size(&ctx, s_rom + (ROM_SIZE - PATCH_BLOCK_SIZE), ROM_SIZE, false,
                         SAVE_MODE_SRAM);
    /* Last data byte at 0xFFEFF: + 4, rounded up to the next 16 bytes. */
    CHECK_EQ(0xFFF10, ctx.st.trim_size);
    CHECK(patch_payload_needs_extra_block(&ctx, ROM_SIZE) == false);
}

TEST(trim_size_full_rom_reserves_payload_space)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    s.save_state_hook = true; /* needs 0x1000 bytes */
    make_rom(ROM_SIZE);
    init(&ctx, &s, NULL, 0);
    patch_find_trim_size(&ctx, s_rom + (ROM_SIZE - PATCH_BLOCK_SIZE), ROM_SIZE, false,
                         SAVE_MODE_SRAM);
    /* No padding: the payload goes after the ROM, still below 8 MiB. */
    CHECK_EQ(ROM_SIZE + 16, ctx.st.trim_size);
    CHECK(patch_payload_needs_extra_block(&ctx, ROM_SIZE));
}

TEST(scan_irq_references_records_both_vectors)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    uint32_t block[64] = {0xEA00002E};
    block[5] = 0x03007FFC;
    block[10] = 0x03FFFFFC;
    make_rom(ROM_SIZE);
    init(&ctx, &s, NULL, 0);
    patch_scan_irq_references(&ctx, block, sizeof(block), 0);
    patch_scan_irq_references(&ctx, block, sizeof(block), PATCH_BLOCK_SIZE);
    CHECK_EQ(4, ctx.st.entry_count);
    CHECK_EQ(0x2E, ctx.st.entry_branch);
    CHECK_EQ(5, ctx.st.entries[0].word_index);
    CHECK_EQ(10, ctx.st.entries[1].word_index);
    CHECK_EQ(PATCH_BLOCK_SIZE / 4 + 5, ctx.st.entries[2].word_index);
    CHECK_EQ(PATCH_IRQ_VECTOR_REPLACEMENT, ctx.st.entries[3].value);
}

TEST(scan_irq_references_caps_entries)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    static uint32_t block[256];
    for (unsigned i = 0; i < 256; i++) {
        block[i] = 0x03007FFC;
    }
    init(&ctx, &s, NULL, 0);
    patch_scan_irq_references(&ctx, block, sizeof(block), 0);
    CHECK_EQ(PATCH_MAX_ENTRIES, ctx.st.entry_count);
}

TEST(irq_database_lookup)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    char code[4];
    memcpy(code, &irq_patch_db[0], 4); /* first game in the table */
    patch_init(&ctx, &PLATFORM, &s_payloads, code, &s, NULL, 0);
    CHECK(patch_use_irq_database(&ctx));
    CHECK_EQ(irq_patch_db[1], ctx.st.entry_count);
    CHECK_EQ(irq_patch_db[2], ctx.st.entries[0].word_index);

    init(&ctx, &s, NULL, 0);
    CHECK(!patch_use_irq_database(&ctx));
}

TEST(state_round_trip_and_hook_check)
{
    patch_context_t ctx;
    patch_context_t loaded;
    settings_t s = default_settings();
    uint32_t words[PATCH_PAT_FILE_SIZE / 4];
    uint32_t block[16] = {0xEA00002E, 0x03007FFC};
    s.sleep_hook = true;
    init(&ctx, &s, NULL, 0);
    patch_scan_irq_references(&ctx, block, sizeof(block), 0);
    ctx.st.trim_size = 0x123450;
    patch_state_serialize(&ctx, words);
    CHECK_EQ(PATCH_PAT_FILE_SIZE, sizeof(words));
    CHECK_EQ(1, words[0]); /* word index of the first entry */
    CHECK_EQ(0x123450, words[PATCH_MAX_ENTRIES * 2 + 6]);

    init(&loaded, &s, NULL, 0);
    CHECK(patch_state_deserialize(&loaded, words, PATCH_PAT_FILE_SIZE / 4));
    CHECK_EQ(1, loaded.st.entry_count);
    CHECK_EQ(0x123450, loaded.st.trim_size);
    CHECK_EQ(0x2E, loaded.st.entry_branch);

    /* A cache built for other hooks is rejected. */
    s.sleep_hook = false;
    init(&loaded, &s, NULL, 0);
    CHECK(!patch_state_deserialize(&loaded, words, PATCH_PAT_FILE_SIZE / 4));
    CHECK(!patch_state_deserialize(&loaded, words, 10));
}

TEST(hooks_psram_installs_sleep_payload)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    s.reset_hook = true;
    s.sleep_hook = true;
    make_rom(0x80000);
    init(&ctx, &s, NULL, 0);
    put_word(s_rom, 0x100, 0x03007FFC);
    patch_scan_irq_references(&ctx, (const uint32_t *)s_rom, ROM_SIZE, 0);
    patch_find_trim_size(&ctx, s_rom + (ROM_SIZE - PATCH_BLOCK_SIZE), ROM_SIZE, false,
                         SAVE_MODE_SRAM);
    uint32_t trim = ctx.st.trim_size;
    patch_apply_hooks_psram(&ctx, (uint32_t *)s_rom, ROM_SIZE);

    CHECK_EQ(0xEA000000u | ((trim - 8) / 4), word_at(s_psram, 0));
    CHECK_EQ(PATCH_IRQ_VECTOR_REPLACEMENT, word_at(s_psram, 0x100));
    /* Payload fields: return to the original entry point, hotkeys. */
    CHECK_EQ(0x08000000u + 0x2E * 4 + 8, word_at(s_psram, trim + 0x00));
    CHECK_EQ(settings_hotkey_mask(s.sleep_keys), word_at(s_psram, trim + 0x08));
    CHECK_EQ(settings_hotkey_mask(s.menu_keys), word_at(s_psram, trim + 0x0C));
    CHECK_EQ(0xA5A5A5A5u, word_at(s_psram, trim + 0x20)); /* payload code copied */
}

TEST(hooks_psram_disabled_hotkey_is_zero)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    s.reset_hook = true; /* sleep off */
    make_rom(0x80000);
    init(&ctx, &s, NULL, 0);
    put_word(s_rom, 0x100, 0x03007FFC);
    patch_scan_irq_references(&ctx, (const uint32_t *)s_rom, ROM_SIZE, 0);
    patch_find_trim_size(&ctx, s_rom + (ROM_SIZE - PATCH_BLOCK_SIZE), ROM_SIZE, false,
                         SAVE_MODE_SRAM);
    patch_apply_hooks_psram(&ctx, (uint32_t *)s_rom, ROM_SIZE);
    CHECK_EQ(0, word_at(s_psram, ctx.st.trim_size + 0x08));
}

TEST(hooks_psram_writes_cheat_table)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    const cheat_code_t cheats[] = {
        {0x0002F3A, 0x64}, /* EWRAM */
        {0x0041234, 0x01}, /* >= 0x40000: IWRAM */
    };
    s.cheats = true;
    make_rom(0x80000);
    init(&ctx, &s, cheats, 2);
    put_word(s_rom, 0x100, 0x03007FFC);
    patch_scan_irq_references(&ctx, (const uint32_t *)s_rom, ROM_SIZE, 0);
    patch_find_trim_size(&ctx, s_rom + (ROM_SIZE - PATCH_BLOCK_SIZE), ROM_SIZE, false,
                         SAVE_MODE_SRAM);
    uint32_t trim = ctx.st.trim_size;
    patch_apply_hooks_psram(&ctx, (uint32_t *)s_rom, ROM_SIZE);

    CHECK_EQ(2, word_at(s_psram, trim + 0x14));
    CHECK_EQ(0x02002F3A, word_at(s_psram, trim + 0x40));
    CHECK_EQ(0x64, word_at(s_psram, trim + 0x44));
    CHECK_EQ(0x03001234, word_at(s_psram, trim + 0x48));
    CHECK_EQ(0x01, word_at(s_psram, trim + 0x4C));
}

TEST(hooks_psram_clamps_cheats_to_work_buffer)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    static cheat_code_t cheats[5000];
    for (unsigned i = 0; i < 5000; i++) {
        cheats[i] = (cheat_code_t){i, i & 0xFF};
    }
    s.cheats = true;
    make_rom(0x80000);
    init(&ctx, &s, cheats, 5000);
    put_word(s_rom, 0x100, 0x03007FFC);
    patch_scan_irq_references(&ctx, (const uint32_t *)s_rom, ROM_SIZE, 0);
    patch_find_trim_size(&ctx, s_rom + (ROM_SIZE - PATCH_BLOCK_SIZE), ROM_SIZE, false,
                         SAVE_MODE_SRAM);
    patch_apply_hooks_psram(&ctx, (uint32_t *)s_rom, ROM_SIZE);
    CHECK_EQ((WORK_SIZE - 0x40) / 8, word_at(s_psram, ctx.st.trim_size + 0x14));
}

TEST(trim_size_for_nor_never_straddles_blocks)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    s.cheats = true; /* 0x2000 bytes */
    make_rom(PATCH_BLOCK_SIZE - 0x100);
    init(&ctx, &s, NULL, 0);
    patch_find_trim_size(&ctx, s_rom + (ROM_SIZE - PATCH_BLOCK_SIZE), ROM_SIZE, true,
                         SAVE_MODE_SRAM);
    CHECK_EQ(0, (ctx.st.trim_size % PATCH_BLOCK_SIZE + 0x2000 > PATCH_BLOCK_SIZE));
}

TEST(hooks_nor_clips_writes_to_block)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    static uint8_t block[PATCH_BLOCK_SIZE + 64];
    s.reset_hook = true;
    make_rom(ROM_SIZE);
    init(&ctx, &s, NULL, 0);
    put_word(s_rom, 0x100, 0x03007FFC);
    patch_scan_irq_references(&ctx, (const uint32_t *)s_rom, ROM_SIZE, 0);
    /* Payload straddling the end of the first block. */
    ctx.st.trim_size = PATCH_BLOCK_SIZE - 0x10;

    memset(block, 0x11, sizeof(block));
    patch_apply_hooks_nor(&ctx, block, 0);
    CHECK_EQ(0xEA000000u | ((PATCH_BLOCK_SIZE - 0x10 - 8) / 4), word_at(block, 0));
    CHECK_EQ(PATCH_IRQ_VECTOR_REPLACEMENT, word_at(block, 0x100));
    CHECK_EQ(0xA5A5A5A5u, word_at(block, PATCH_BLOCK_SIZE - 0xC)); /* payload code */
    CHECK_EQ(settings_hotkey_mask(s.menu_keys), word_at(block, PATCH_BLOCK_SIZE - 4));
    CHECK_EQ(0x11111111u, word_at(block, PATCH_BLOCK_SIZE)); /* untouched */
    CHECK_EQ(0, s_writes);                                   /* nothing went to PSRAM */

    /* Later blocks don't get the entry branch. */
    memset(block, 0x11, sizeof(block));
    patch_apply_hooks_nor(&ctx, block, PATCH_BLOCK_SIZE);
    CHECK_EQ(0x11111111u, word_at(block, 0));
}

TEST(trim_size_for_full_32mb_rom_in_nor_stays_inside_cartridge)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    s.cheats = true;    /* 0x2000 bytes */
    make_rom(ROM_SIZE); /* no padding at all */
    init(&ctx, &s, NULL, 0);
    patch_find_trim_size(&ctx, s_rom + (ROM_SIZE - PATCH_BLOCK_SIZE), 0x2000000, true,
                         SAVE_MODE_SRAM);
    CHECK(ctx.st.trim_size + 0x2000 <= 0x2000000);
}

TEST(hooks_nor_cheat_count_matches_written_table)
{
    patch_context_t ctx;
    settings_t s = default_settings();
    static uint8_t block[PATCH_BLOCK_SIZE];
    static cheat_code_t cheats[1000];
    for (unsigned i = 0; i < 1000; i++) {
        cheats[i] = (cheat_code_t){0x100 + i, i & 0xFF};
    }
    s.cheats = true;
    make_rom(ROM_SIZE);
    init(&ctx, &s, cheats, 1000);
    put_word(s_rom, 0x100, 0x03007FFC);
    patch_scan_irq_references(&ctx, (const uint32_t *)s_rom, ROM_SIZE, 0);
    ctx.st.trim_size = PATCH_BLOCK_SIZE - 0xE00; /* room for (0xE00 - 0x40) / 8 codes */

    memcpy(block, s_rom, sizeof(block));
    patch_apply_hooks_nor(&ctx, block, 0);
    uint32_t count = word_at(block, ctx.st.trim_size + 0x14);
    CHECK_EQ((0xE00 - 0x40) / 8, count);
    /* The last promised code is really there. */
    CHECK_EQ(0x02000100u + count - 1, word_at(block, ctx.st.trim_size + 0x40 + 8 * (count - 1)));
}

SUITE(patch)
{
    RUN(trim_size_follows_last_data_byte);
    RUN(trim_size_full_rom_reserves_payload_space);
    RUN(scan_irq_references_records_both_vectors);
    RUN(scan_irq_references_caps_entries);
    RUN(irq_database_lookup);
    RUN(state_round_trip_and_hook_check);
    RUN(hooks_psram_installs_sleep_payload);
    RUN(hooks_psram_disabled_hotkey_is_zero);
    RUN(hooks_psram_writes_cheat_table);
    RUN(hooks_psram_clamps_cheats_to_work_buffer);
    RUN(trim_size_for_nor_never_straddles_blocks);
    RUN(hooks_nor_clips_writes_to_block);
    RUN(trim_size_for_full_32mb_rom_in_nor_stays_inside_cartridge);
    RUN(hooks_nor_cheat_count_matches_written_table);
}
