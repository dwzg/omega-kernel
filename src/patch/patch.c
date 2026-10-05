/**
 * @file patch.c
 * @brief Patch engine. See patch.h and docs/patching.md.
 *
 * Function names of the original kernel are given in brackets so the two
 * implementations can be compared.
 */
#include "patch/patch.h"

#include <string.h>

#include "patch/irq_patch_db.h"
#include "patch/payloads.h"
#include "patch/rom_fixes.h"

/** Words of the BIOS IRQ vector that games store in literal pools. */
#define IRQ_VECTOR 0x03007FFCu
#define IRQ_VECTOR_MIRROR 0x03FFFFFCu

/** Bytes reserved after the trim size for each payload. */
#define PAYLOAD_SPACE_SLEEP 0x300u
#define PAYLOAD_SPACE_SAVE_STATE 0x1000u
#define PAYLOAD_SPACE_CHEATS 0x2000u

#define ARM_BRANCH 0xEA000000u
#define CART_BASE 0x08000000u
#define MAX_ROM_SIZE 0x02000000u

/* ------------------------------------------------------------------------- */
/* Writing                                                                   */
/* ------------------------------------------------------------------------- */

/**
 * Write into the game image [Write]. In NOR mode only the part that falls
 * into the current block is written.
 */
static void rom_write(patch_context_t *ctx, uint32_t rom_offset, const void *data, uint32_t size)
{
    const patch_state_t *st = &ctx->st;
    if (!st->nor_mode) {
        ctx->platform->write_psram(rom_offset, data, size);
        return;
    }
    if (rom_offset < st->window_offset || rom_offset >= st->window_offset + PATCH_BLOCK_SIZE) {
        return;
    }
    uint32_t start = rom_offset - st->window_offset;
    if (size > PATCH_BLOCK_SIZE - start) {
        size = PATCH_BLOCK_SIZE - start; /* never write past the block buffer */
    }
    const uint16_t *src = data;
    volatile uint16_t *dst = (volatile uint16_t *)(ctx->window + start);
    for (uint32_t i = 0; i < size / 2; i++) {
        dst[i] = src[i];
    }
}

static void rom_write32(patch_context_t *ctx, uint32_t rom_offset, uint32_t value)
{
    rom_write(ctx, rom_offset, &value, sizeof(value));
}

static void rom_write16(patch_context_t *ctx, uint32_t rom_offset, uint16_t value)
{
    rom_write(ctx, rom_offset, &value, sizeof(value));
}

/** Copy a payload into the work buffer with halfword accesses (VRAM-safe). */
static uint8_t *stage_payload(patch_context_t *ctx, const payload_t *p)
{
    volatile uint16_t *dst = (volatile uint16_t *)ctx->platform->work;
    const uint16_t *src = (const uint16_t *)p->code;
    for (uint32_t i = 0; i < p->size / 2; i++) {
        dst[i] = src[i];
    }
    return ctx->platform->work;
}

static void set_word(uint8_t *buffer, uint32_t offset, uint32_t value)
{
    *(volatile uint32_t *)(buffer + offset) = value;
}

/* ------------------------------------------------------------------------- */
/* State                                                                     */
/* ------------------------------------------------------------------------- */

void patch_init(patch_context_t *ctx, const patch_platform_t *platform,
                const patch_payloads_t *payloads, const char game_code[4],
                const settings_t *settings, const cheat_code_t *cheats, size_t cheat_count)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->platform = platform;
    ctx->payloads = payloads;
    memcpy(&ctx->game_code, game_code, 4);
    ctx->settings = *settings;
    ctx->cheats = cheats;
    ctx->cheat_count = cheat_count;
    ctx->st.auto_save = true;
}

void patch_state_serialize(const patch_context_t *ctx, uint32_t words[PATCH_PAT_FILE_SIZE / 4])
{
    const patch_state_t *st = &ctx->st;
    uint32_t *w = words;

    memset(words, 0, PATCH_PAT_FILE_SIZE);
    for (unsigned i = 0; i < PATCH_MAX_ENTRIES; i++) {
        *w++ = st->entries[i].word_index;
        *w++ = st->entries[i].value;
    }
    *w++ = st->nor_mode;
    *w++ = st->window_offset;
    *w++ = st->nes_variant;
    *w++ = st->nes_index;
    *w++ = st->word_offset;
    *w++ = st->entry_count;
    *w++ = st->trim_size;
    *w++ = st->entry_branch;
    /* The cache remembers the hooks it was built for. */
    *w++ = ctx->settings.reset_hook;
    *w++ = ctx->settings.save_state_hook;
    *w++ = ctx->settings.sleep_hook;
    *w++ = ctx->settings.cheats;
}

bool patch_state_deserialize(patch_context_t *ctx, const uint32_t *words, size_t word_count)
{
    patch_state_t *st = &ctx->st;
    patch_state_t loaded = *st;
    const uint32_t *w = words;

    if (word_count < PATCH_MAX_ENTRIES * 2 + 12) {
        return false;
    }
    for (unsigned i = 0; i < PATCH_MAX_ENTRIES; i++) {
        loaded.entries[i].word_index = *w++;
        loaded.entries[i].value = *w++;
    }
    loaded.nor_mode = *w++;
    loaded.window_offset = *w++;
    loaded.nes_variant = *w++;
    loaded.nes_index = *w++;
    loaded.word_offset = *w++;
    loaded.entry_count = *w++;
    loaded.trim_size = *w++;
    loaded.entry_branch = *w++;
    loaded.hook_reset = *w++;
    loaded.hook_save_state = *w++;
    loaded.hook_sleep = *w++;
    loaded.hook_cheats = *w++;

    if (loaded.hook_reset != ctx->settings.reset_hook ||
        loaded.hook_save_state != ctx->settings.save_state_hook ||
        loaded.hook_sleep != ctx->settings.sleep_hook ||
        loaded.hook_cheats != ctx->settings.cheats || loaded.entry_count > PATCH_MAX_ENTRIES) {
        return false;
    }
    *st = loaded;
    return true;
}

/** Record one word replacement [Add2]. Indices are relative to the current block. */
static void add_entry(patch_context_t *ctx, uint32_t word_index, uint32_t value)
{
    patch_state_t *st = &ctx->st;
    if (st->entry_count < PATCH_MAX_ENTRIES) {
        st->entries[st->entry_count].word_index = word_index + st->word_offset;
        st->entries[st->entry_count].value = value;
        st->entry_count++;
    }
}

/* ------------------------------------------------------------------------- */
/* Analysis                                                                  */
/* ------------------------------------------------------------------------- */

static uint32_t payload_space(const settings_t *s)
{
    if (s->cheats) {
        return PAYLOAD_SPACE_CHEATS;
    }
    return s->save_state_hook ? PAYLOAD_SPACE_SAVE_STATE : PAYLOAD_SPACE_SLEEP;
}

/* [SetTrimSize] + [Patch_SpecialROM_TrimSize] */
void patch_find_trim_size(patch_context_t *ctx, const uint8_t *last_block, uint32_t rom_size,
                          bool for_nor, save_mode_t save_mode)
{
    patch_state_t *st = &ctx->st;
    uint32_t space = payload_space(&ctx->settings);

    /* Scan backwards from the end for the first byte that differs from the
     * padding value; the payload goes just after it. */
    uint32_t bottom = (rom_size - 1) % PATCH_BLOCK_SIZE;
    if (bottom) {
        bottom--;
    }
    uint8_t padding = last_block[bottom];
    uint32_t top = (bottom > space + 16) ? bottom - space - 16 : 0;
    uint32_t aligned_size = rom_size + (16 - (rom_size & 15));

    for (uint32_t i = bottom;; i--) {
        if (last_block[i] != padding || i == top) {
            st->trim_size = i + 4 + ((rom_size - 1) & ~(PATCH_BLOCK_SIZE - 1));
            st->trim_size += 16 - (st->trim_size & 15);
            if (st->trim_size > aligned_size) {
                st->trim_size = aligned_size;
            }
            break;
        }
    }

    bool eeprom = save_mode == SAVE_MODE_EEPROM_512 || save_mode == SAVE_MODE_EEPROM_8K;
    if (for_nor) {
        /* The payload must not straddle two flash blocks. */
        if ((st->trim_size & (PATCH_BLOCK_SIZE - 1)) + space > PATCH_BLOCK_SIZE) {
            st->trim_size =
                ((rom_size + PATCH_BLOCK_SIZE - 1) / PATCH_BLOCK_SIZE) * PATCH_BLOCK_SIZE;
        }
        if (rom_size <= 0x1000000 && st->trim_size + space > 0x1000000) {
            /* EEPROM is mapped above 16 MiB in small ROMs. */
            st->trim_size = eeprom ? 0x1000000 - space : 0x1000000;
        } else if (st->trim_size + space > MAX_ROM_SIZE) {
            /* A full 32 MiB ROM: the payload must still be inside the cartridge. */
            st->trim_size = MAX_ROM_SIZE - space;
        }
    } else if (rom_size <= 0x800000) {
        if (st->trim_size + space > 0x800000) {
            st->trim_size = 0x800000;
        }
    } else if (rom_size <= 0x1000000) {
        if (st->trim_size + space > 0x1000000) {
            st->trim_size = eeprom ? 0x1000000 - space : 0x1000000;
        }
    } else if (st->trim_size + space > MAX_ROM_SIZE) {
        st->trim_size = MAX_ROM_SIZE - space;
    }

    for (size_t i = 0; i < ROM_TRIM_OVERRIDE_COUNT; i++) {
        if (ROM_TRIM_OVERRIDES[i].game_code == ctx->game_code) {
            st->trim_size = ROM_TRIM_OVERRIDES[i].trim_size;
            break;
        }
    }
}

/* [PatchInternal] */
void patch_scan_irq_references(patch_context_t *ctx, const uint32_t *block, uint32_t size,
                               uint32_t offset)
{
    patch_state_t *st = &ctx->st;
    st->word_offset = offset / 4;
    if (offset == 0) {
        st->entry_branch = block[0] & 0x00FFFFFF;
    }
    for (uint32_t i = 0; i < size / 4; i++) {
        if (block[i] == IRQ_VECTOR || block[i] == IRQ_VECTOR_MIRROR) {
            add_entry(ctx, i, PATCH_IRQ_VECTOR_REPLACEMENT);
        }
    }
}

/* [use_internal_engine] */
bool patch_use_irq_database(patch_context_t *ctx)
{
    patch_state_t *st = &ctx->st;
    st->word_offset = 0;

    size_t i = 0;
    while (i + 1 < irq_patch_db_words && irq_patch_db[i] != IRQ_PATCH_DB_END) {
        uint32_t count = irq_patch_db[i + 1];
        if (irq_patch_db[i] == ctx->game_code) {
            st->entry_count = 0;
            for (uint32_t k = 0; k < count && i + 2 + k < irq_patch_db_words; k++) {
                add_entry(ctx, irq_patch_db[i + 2 + k], PATCH_IRQ_VECTOR_REPLACEMENT);
            }
            return true;
        }
        i += 2 + count;
    }
    return false;
}

/* [Patch_SpecialROM_sleepmode] */
void patch_add_game_irq_fixes(patch_context_t *ctx)
{
    ctx->st.word_offset = 0;
    for (size_t i = 0; i < ROM_IRQ_FIX_COUNT; i++) {
        if (ROM_IRQ_FIXES[i].game_code == ctx->game_code) {
            add_entry(ctx, ROM_IRQ_FIXES[i].rom_offset / 4, ROM_IRQ_FIXES[i].value);
        }
    }
}

/**
 * Detect PocketNES ROM builds [CheckNes]. PocketNES starts with a branch to
 * a fixed setup sequence; two builds exist that differ in one instruction.
 */
static void detect_pocketnes(patch_context_t *ctx, const uint32_t *data, uint32_t words)
{
    static const uint32_t SIGNATURE[] = {
        0xE8B503D3, 0xE129F007, 0xE281DEBA, 0xE129F008, 0xE281DEBE,
        0xE129F009, 0xE281DC0B, 0xE92D0003, 0xEF110000, 0xE8BD8001,
    };
    patch_state_t *st = &ctx->st;
    uint32_t jump = data[0];

    if ((jump & 0xFF000000) != ARM_BRANCH) {
        return;
    }
    st->nes_index = (jump & 0x00FFFFFF) + 2;
    uint32_t n = st->nes_index;
    bool match = n + 10 < words && (data[n] & 0xFFFFFF00) == 0xE28F5000;
    for (unsigned k = 0; match && k < 10; k++) {
        match = data[n + 1 + k] == SIGNATURE[k];
    }
    if (!match) {
        st->nes_variant = 0;
    } else if (data[n] == 0xE28F503C) {
        st->nes_variant = 1;
    } else if (data[n] == 0xE28F5040) {
        st->nes_variant = 2;
    }
}

/** Install the PocketNES IRQ fix [PatchNes]. */
static void patch_pocketnes(patch_context_t *ctx, const uint32_t *data, uint32_t words)
{
    patch_state_t *st = &ctx->st;
    if (!st->nes_variant) {
        return;
    }
    rom_write32(ctx, 36 + st->nes_index * 4, ARM_BRANCH | (0x3FDF5 - st->nes_index));
    rom_write(ctx, 0xFF800, ctx->payloads->nes_patch, ctx->payloads->nes_patch_size);
    if (st->window_offset == 0) {
        uint32_t k = st->nes_index + 17 + st->nes_variant - 1;
        if (k < words) {
            st->nes_entry_word = data[k];
        }
    }
    rom_write32(ctx, 0xFF840, st->nes_entry_word);

    uint32_t target = (st->nes_entry_word - CART_BASE) / 4 - 1;
    uint32_t local = target - st->window_offset / 4;
    if (target >= st->window_offset / 4 && local < words && data[local] == 0x3032) {
        rom_write32(ctx, 0xFF86C, 0x060000F8);
    }
}

/* [PatchDragonBallZ] */
static void apply_write_fixes(patch_context_t *ctx)
{
    for (size_t i = 0; i < ROM_WRITE_FIX_COUNT; i++) {
        const rom_write_fix_t *fix = &ROM_WRITE_FIXES[i];
        if (fix->game_code != ctx->game_code) {
            continue;
        }
        for (size_t k = 0; k < fix->count; k++) {
            rom_write16(ctx, fix->writes[k].rom_offset, fix->writes[k].value);
        }
        return;
    }
}

/* [Check_Fire_Emblem] */
static void apply_fire_emblem_fix(patch_context_t *ctx)
{
    const fire_emblem_fix_t *fix = NULL;
    for (size_t i = 0; i < ROM_FIRE_EMBLEM_FIX_COUNT; i++) {
        if (ROM_FIRE_EMBLEM_FIXES[i].game_code == ctx->game_code) {
            fix = &ROM_FIRE_EMBLEM_FIXES[i];
            break;
        }
    }
    if (!fix) {
        ctx->st.auto_save = true;
        return;
    }

    for (unsigned i = 0; i < 5; i++) {
        rom_write32(ctx, fix->call_sites[i], 0x47004800); /* ldr r0,[pc]; bx r0 */
        rom_write32(ctx, fix->call_sites[i] + 4,
                    CART_BASE + fix->payload_offset + fix->entry_offsets[i]);
    }
    const payload_t *p = &ctx->payloads->fire_emblem[fix->payload];
    uint8_t *buffer = stage_payload(ctx, p);
    if (fix->modify_value && p->modify_address != PAYLOAD_NO_FIELD) {
        set_word(buffer, p->modify_address, fix->modify_value);
    }
    rom_write(ctx, fix->payload_offset, buffer, p->size);
    ctx->st.auto_save = false;
}

/* [Patch_somegame]: lower an IWRAM size word that collides with the payload. */
static void apply_iwram_size_fix(patch_context_t *ctx, const uint32_t *data)
{
    if (ctx->game_code != ROM_FIX_IWRAM_SIZE_GAME) {
        return;
    }
    for (uint32_t i = 0; i < 0x100; i++) {
        if (data[i] == 0x03000000 && data[i + 1] == 0x8000) {
            rom_write32(ctx, (i + 1) * 4, 0x7FF0);
        }
    }
}

/**
 * Move the game's stack down to make room for the payload [Get_spend_address].
 * Finds "mov r0,#0x1F; msr cpsr,r0; ldr sp,[pc,#x]" near the entry point and
 * lowers the loaded value by 0x80. Modifies @p data in place.
 * @return The new stack address, or 0 if nothing was changed.
 */
static uint32_t relocate_stack(uint32_t *data)
{
    const uint32_t search_words = 0x5000 / 4;
    uint32_t i;
    uint32_t offset = 0;
    uint32_t direction = 0;

    for (i = 0; i < search_words; i++) {
        if ((data[i] & 0xFFFF001F) == 0xE3A0001F &&
            (data[i + 1] == 0xE129F000 || data[i + 1] == 0xE121F000)) {
            offset = data[i + 2] & 0xFFF;
            direction = data[i + 2] & 0x00F00000;
            break;
        }
    }
    if (i == search_words) {
        return 0;
    }

    uint32_t address;
    if (direction == 0x00900000) {
        address = i * 4 + offset + 16;
    } else if (direction == 0x00100000) {
        address = i * 4 - offset + 16;
    } else {
        return 0;
    }
    uint32_t *sp = &data[address / 4];
    if (*sp > 0x03007E80 || *sp == 0x0203FFFC) {
        *sp -= 0x80;
        return *sp;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Hook installation                                                         */
/* ------------------------------------------------------------------------- */

/** Branch from the ROM entry point into the payload, plus all entries [Patch_B_address]. */
static void write_entries(patch_context_t *ctx)
{
    patch_state_t *st = &ctx->st;
    if (st->entry_count == 0) {
        return;
    }
    rom_write32(ctx, 0, ARM_BRANCH | ((st->trim_size - 8) / 4));
    for (uint32_t i = 0; i < st->entry_count; i++) {
        rom_write32(ctx, st->entries[i].word_index * 4, st->entries[i].value);
    }
}

static uint32_t return_address(const patch_state_t *st)
{
    return CART_BASE + st->entry_branch * 4 + 8;
}

static uint32_t clamp_to_rom(const patch_state_t *st, uint32_t size)
{
    if (st->trim_size >= MAX_ROM_SIZE) {
        return 0;
    }
    return (st->trim_size + size > MAX_ROM_SIZE) ? MAX_ROM_SIZE - st->trim_size : size;
}

/* [Patch_Reset_Sleep] */
static void install_sleep_payload(patch_context_t *ctx)
{
    const payload_t *p = &ctx->payloads->sleep;
    const settings_t *s = &ctx->settings;

    write_entries(ctx);
    uint8_t *buffer = stage_payload(ctx, p);
    set_word(buffer, p->return_address, return_address(&ctx->st));
    set_word(buffer, p->key_b, s->reset_hook ? settings_hotkey_mask(s->menu_keys) : 0);
    set_word(buffer, p->key_a, s->sleep_hook ? settings_hotkey_mask(s->sleep_keys) : 0);
    rom_write(ctx, ctx->st.trim_size, buffer, p->size);
}

/** Map a cheat address from the file to a GBA bus address. */
static uint32_t cheat_bus_address(uint32_t address)
{
    if (address >= 0x40000) {
        return (address & 0x7FFF) + 0x03000000; /* IWRAM */
    }
    return (address & 0x3FFFF) + 0x02000000; /* EWRAM */
}

/* [Patch_RTS_Cheat] */
static void install_full_payload(patch_context_t *ctx)
{
    const payload_t *p = &ctx->payloads->full;
    const settings_t *s = &ctx->settings;
    patch_state_t *st = &ctx->st;

    write_entries(ctx);
    uint8_t *buffer = stage_payload(ctx, p);
    set_word(buffer, p->return_address, return_address(st));
    if (st->stack_address) {
        set_word(buffer, p->return_address + 4, st->stack_address);
    }
    set_word(buffer, p->key_b, settings_hotkey_mask(s->menu_keys));
    set_word(buffer, p->key_a, s->sleep_hook ? settings_hotkey_mask(s->sleep_keys) : 0);
    set_word(buffer, p->save_state_switch, s->save_state_hook);

    /* The cheat table must stay inside the work buffer, and in NOR inside the
     * block that holds the payload (later blocks would not receive it, while
     * the count word would still promise it). */
    size_t max_cheats = (ctx->platform->work_size - p->cheat_table) / 8;
    if (st->nor_mode) {
        uint32_t room = PATCH_BLOCK_SIZE - st->trim_size % PATCH_BLOCK_SIZE;
        size_t fit = room > p->cheat_table ? (room - p->cheat_table) / 8 : 0;
        if (fit < max_cheats) {
            max_cheats = fit;
        }
    }
    size_t count = ctx->cheat_count < max_cheats ? ctx->cheat_count : max_cheats;
    set_word(buffer, p->cheat_count, (uint32_t)count);
    for (size_t i = 0; i < count; i++) {
        set_word(buffer, p->cheat_table + 8 * i, cheat_bus_address(ctx->cheats[i].address));
        set_word(buffer, p->cheat_table + 8 * i + 4, ctx->cheats[i].value);
    }

    uint32_t size = clamp_to_rom(st, p->size_without_cheats + (uint32_t)count * 8);
    rom_write(ctx, st->trim_size, buffer, size);
}

/* [Patch_RTS_only] */
static void install_save_state_payload(patch_context_t *ctx)
{
    const payload_t *p = &ctx->payloads->save_state_only;
    const settings_t *s = &ctx->settings;
    patch_state_t *st = &ctx->st;

    write_entries(ctx);
    uint8_t *buffer = stage_payload(ctx, p);
    set_word(buffer, p->return_address, return_address(st));
    if (st->stack_address) {
        set_word(buffer, p->return_address + 4, st->stack_address);
    }
    set_word(buffer, p->key_a, settings_hotkey_mask(s->sleep_keys));
    set_word(buffer, p->key_b, settings_hotkey_mask(s->menu_keys));
    rom_write(ctx, st->trim_size, buffer, clamp_to_rom(st, p->size));
}

static void install_payload(patch_context_t *ctx)
{
    const settings_t *s = &ctx->settings;
    if (settings_save_state_only(s)) {
        install_save_state_payload(ctx);
    } else if (s->save_state_hook || (s->cheats && ctx->cheat_count > 0)) {
        install_full_payload(ctx);
    } else {
        install_sleep_payload(ctx);
    }
}

static bool needs_stack_relocation(const patch_context_t *ctx)
{
    const settings_t *s = &ctx->settings;
    return s->save_state_hook || (s->cheats && ctx->cheat_count > 0);
}

/* [GBApatch_Cleanrom] */
void patch_apply_clean_psram(patch_context_t *ctx, uint32_t *rom, uint32_t rom_size)
{
    uint32_t words = rom_size / 4;
    ctx->st.window_offset = 0;
    ctx->st.nor_mode = 0;
    detect_pocketnes(ctx, rom, words);
    patch_pocketnes(ctx, rom, words);
    apply_write_fixes(ctx);
    apply_fire_emblem_fix(ctx);
}

/* [GBApatch_PSRAM] */
void patch_apply_hooks_psram(patch_context_t *ctx, uint32_t *rom, uint32_t rom_size)
{
    uint32_t words = rom_size / 4;
    ctx->st.window_offset = 0;
    ctx->st.nor_mode = 0;
    ctx->st.entry_branch = rom[0] & 0x00FFFFFF;

    detect_pocketnes(ctx, rom, words);
    patch_pocketnes(ctx, rom, words);
    apply_write_fixes(ctx);
    apply_fire_emblem_fix(ctx);
    apply_iwram_size_fix(ctx, rom);
    if (needs_stack_relocation(ctx)) {
        ctx->st.stack_address = relocate_stack(rom);
    }
    install_payload(ctx);
}

/* [GBApatch_Cleanrom_NOR] */
void patch_apply_clean_nor(patch_context_t *ctx, uint8_t *block, uint32_t offset)
{
    const uint32_t words = PATCH_BLOCK_SIZE / 4;
    ctx->window = block;
    ctx->st.window_offset = offset;
    ctx->st.nor_mode = 1;
    if (offset == 0) {
        detect_pocketnes(ctx, (const uint32_t *)block, words);
    }
    patch_pocketnes(ctx, (const uint32_t *)block, words);
    apply_write_fixes(ctx);
    apply_fire_emblem_fix(ctx);
}

/* [GBApatch_NOR] */
void patch_apply_hooks_nor(patch_context_t *ctx, uint8_t *block, uint32_t offset)
{
    const uint32_t words = PATCH_BLOCK_SIZE / 4;
    uint32_t *data = (uint32_t *)block;
    patch_state_t *st = &ctx->st;

    ctx->window = block;
    st->window_offset = offset;
    st->nor_mode = 1;
    if (offset == 0) {
        detect_pocketnes(ctx, data, words);
        st->entry_branch = data[0] & 0x00FFFFFF;
        rom_write32(ctx, 0, ARM_BRANCH | ((st->trim_size - 8) / 4));
        st->stack_address = relocate_stack(data);
        apply_iwram_size_fix(ctx, data);
    }
    patch_pocketnes(ctx, data, words);
    apply_write_fixes(ctx);
    apply_fire_emblem_fix(ctx);
    install_payload(ctx);
}

bool patch_payload_needs_extra_block(const patch_context_t *ctx, uint32_t rom_size)
{
    uint32_t rounded = ((rom_size + PATCH_BLOCK_SIZE - 1) / PATCH_BLOCK_SIZE) * PATCH_BLOCK_SIZE;
    return ctx->st.trim_size >= rounded;
}
