/**
 * @file patch.h
 * @brief Patch engine: modifies a GBA ROM image so the kernel's in-game hooks
 * (reset to menu, sleep, real-time save states, cheats) run inside the game.
 *
 * How the hooks are installed (see docs/patching.md for the full story):
 *
 *  1. **Trim size** - the end of the ROM's real data is found (padding bytes
 *     are trimmed). The hook payload is appended there.
 *  2. **IRQ redirection** - every reference to the BIOS IRQ vector
 *     (0x03007FFC) is changed to 0x03007FF4, a slot the payload owns. The
 *     references are found by scanning the ROM, or taken from a database
 *     ("fast patch engine").
 *  3. **Entry point** - the ROM's first instruction is replaced by a branch
 *     into the payload, which installs its IRQ handler and then jumps to the
 *     game's original entry point.
 *  4. **Game-specific fixes** - some games need extra changes (tables in
 *     rom_fixes.c).
 *
 * The engine works on two kinds of targets:
 *  - **PSRAM**: the whole ROM is already in PSRAM; patches are written there.
 *  - **NOR window**: the ROM is streamed in 128 KiB blocks while it is
 *    written to NOR; patches that fall inside the current block are applied
 *    to the block buffer.
 *
 * All hardware access goes through ::patch_platform_t, so the engine is
 * portable and unit-tested on the host.
 */
#ifndef PATCH_PATCH_H
#define PATCH_PATCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/cheat.h"
#include "core/save_type.h"
#include "core/settings.h"

/** Maximum number of IRQ-reference patches. */
#define PATCH_MAX_ENTRIES 32
/** Size of one NOR block, the unit in which ROMs are streamed. */
#define PATCH_BLOCK_SIZE 0x20000u
/** Value that IRQ-vector references are rewritten to. */
#define PATCH_IRQ_VECTOR_REPLACEMENT 0x03007FF4u

/** One 32-bit word to replace: ROM word index and new value. */
typedef struct {
    uint32_t word_index;
    uint32_t value;
} patch_entry_t;

/**
 * Engine state. The fields up to and including @ref hook_cheats are saved in
 * the `.pat` cache file, in this order, as 32-bit little-endian words (see
 * patch_state_serialize()); do not reorder them.
 */
typedef struct {
    patch_entry_t entries[PATCH_MAX_ENTRIES]; /**< IRQ references to rewrite. */
    uint32_t nor_mode;                        /**< 1 while streaming to NOR. */
    uint32_t window_offset;                   /**< ROM offset of the current NOR block. */
    uint32_t nes_variant;                     /**< 0, or 1/2 for PocketNES builds (see CheckNes). */
    uint32_t nes_index;                       /**< Word index of the PocketNES startup code. */
    uint32_t word_offset;  /**< Added to entry indices found in the current block. */
    uint32_t entry_count;  /**< Used entries in @ref entries. */
    uint32_t trim_size;    /**< Where the payload is placed. */
    uint32_t entry_branch; /**< 24-bit offset of the ROM's original entry branch. */
    uint32_t hook_reset;   /**< Hook flags the cache was built for. */
    uint32_t hook_save_state;
    uint32_t hook_sleep;
    uint32_t hook_cheats;
    /* --- not saved --- */
    uint32_t nes_entry_word; /**< Saved PocketNES word for NOR streaming. */
    uint32_t stack_address;  /**< Relocated stack top, 0 = unchanged. */
    bool auto_save;          /**< Whether the FPGA should auto-save (false for some games). */
} patch_state_t;

/** Number of bytes written to a `.pat` file. */
#define PATCH_PAT_FILE_SIZE (PATCH_MAX_ENTRIES * 8 + 16 * 4)

/** Services the engine needs from the platform. */
typedef struct {
    /** Write @p size bytes (even) at ROM offset @p rom_offset in PSRAM. */
    void (*write_psram)(uint32_t rom_offset, const void *data, uint32_t size);
    /**
     * Scratch memory for assembling payloads; must accept 16/32-bit writes
     * (VRAM on the GBA). At least @ref work_size bytes.
     */
    uint8_t *work;
    uint32_t work_size;
} patch_platform_t;

/** Description of an assembled hook payload (see payloads.h). */
typedef struct patch_payloads patch_payloads_t;

/** Everything the engine needs for one game. */
typedef struct {
    patch_state_t st;
    const patch_platform_t *platform;
    const patch_payloads_t *payloads;
    uint32_t game_code;  /**< ROM header game code as a little-endian word. */
    settings_t settings; /**< Hooks and hotkeys to install. */
    const cheat_code_t *cheats;
    size_t cheat_count;
    uint8_t *window; /**< NOR mode: buffer holding the current block. */
} patch_context_t;

/** @brief Start patching a game: clear all state. */
void patch_init(patch_context_t *ctx, const patch_platform_t *platform,
                const patch_payloads_t *payloads, const char game_code[4],
                const settings_t *settings, const cheat_code_t *cheats, size_t cheat_count);

/**
 * @brief Load a `.pat` cache. The cache is only used if it was built for the
 * same hook settings; otherwise the state is reset.
 * @return true if the cache was used.
 */
bool patch_state_deserialize(patch_context_t *ctx, const uint32_t *words, size_t word_count);

/** @brief Write the `.pat` cache contents (@ref PATCH_PAT_FILE_SIZE bytes). */
void patch_state_serialize(const patch_context_t *ctx, uint32_t words[PATCH_PAT_FILE_SIZE / 4]);

/**
 * @brief Find where the payload goes.
 * @param last_block  The ROM's last 128 KiB block (the one containing byte rom_size - 1).
 * @param rom_size    ROM size in bytes.
 * @param for_nor     The ROM will be written to NOR (payload must not straddle blocks).
 * @param save_mode   Save type; EEPROM games must keep the payload below 16 MiB.
 */
void patch_find_trim_size(patch_context_t *ctx, const uint8_t *last_block, uint32_t rom_size,
                          bool for_nor, save_mode_t save_mode);

/**
 * @brief Scan a block of the ROM for IRQ-vector references.
 * @param block   Block data (word aligned).
 * @param size    Block size in bytes.
 * @param offset  ROM offset of the block.
 */
void patch_scan_irq_references(patch_context_t *ctx, const uint32_t *block, uint32_t size,
                               uint32_t offset);

/**
 * @brief Take the IRQ references from the built-in database (fast patch engine).
 * @return false if the game is not in the database (scan instead).
 */
bool patch_use_irq_database(patch_context_t *ctx);

/** @brief Add the extra entries some games need when the sleep hook is used. */
void patch_add_game_irq_fixes(patch_context_t *ctx);

/** @brief Apply game fixes needed even without hooks ("clean boot") to a ROM in PSRAM. */
void patch_apply_clean_psram(patch_context_t *ctx, uint32_t *rom, uint32_t rom_size);

/** @brief Install the hooks into a ROM in PSRAM. */
void patch_apply_hooks_psram(patch_context_t *ctx, uint32_t *rom, uint32_t rom_size);

/** @brief Apply clean-boot fixes to one block streamed to NOR. */
void patch_apply_clean_nor(patch_context_t *ctx, uint8_t *block, uint32_t offset);

/**
 * @brief Install the hooks into one block streamed to NOR.
 * Call for every block, then once more for the block after the ROM if the
 * payload lies beyond the ROM data (see patch_payload_needs_extra_block()).
 */
void patch_apply_hooks_nor(patch_context_t *ctx, uint8_t *block, uint32_t offset);

/** @brief true if the payload starts at or after the end of the ROM's blocks. */
bool patch_payload_needs_extra_block(const patch_context_t *ctx, uint32_t rom_size);

#endif /* PATCH_PATCH_H */
