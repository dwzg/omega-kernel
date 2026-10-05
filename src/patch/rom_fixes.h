/**
 * @file rom_fixes.h
 * @brief Tables of game-specific patch data.
 *
 * Game codes are the 4 ASCII characters at ROM offset 0xAC read as a
 * little-endian 32-bit word (e.g. "A2CE" -> 0x45433241).
 */
#ifndef PATCH_ROM_FIXES_H
#define PATCH_ROM_FIXES_H

#include <stddef.h>
#include <stdint.h>

#include "patch/payloads.h"

/** Extra IRQ-related word replacements needed with the hooks. */
typedef struct {
    uint32_t game_code;
    uint32_t rom_offset; /**< Byte offset (word aligned). */
    uint32_t value;
} irq_fix_t;

/** Fixed payload location for games whose padding detection fails. */
typedef struct {
    uint32_t game_code;
    uint32_t trim_size;
} trim_override_t;

/** One halfword write. */
typedef struct {
    uint32_t rom_offset;
    uint16_t value;
} rom_write16_t;

/** Halfword writes applied to a game on every boot (clean or with hooks). */
typedef struct {
    uint32_t game_code;
    const rom_write16_t *writes;
    size_t count;
} rom_write_fix_t;

/** Fire Emblem save fix: redirect five save routines into a payload. */
typedef struct {
    uint32_t game_code;
    uint32_t call_sites[5];   /**< Where "ldr r0,[pc]; bx r0" + address are written. */
    uint32_t payload_offset;  /**< Where the payload is written. */
    uint8_t entry_offsets[5]; /**< Thumb entry points inside the payload. */
    fe_payload_id_t payload;
    uint32_t modify_value; /**< 0 = payload has no game-specific word. */
} fire_emblem_fix_t;

extern const irq_fix_t ROM_IRQ_FIXES[];
extern const size_t ROM_IRQ_FIX_COUNT;
extern const trim_override_t ROM_TRIM_OVERRIDES[];
extern const size_t ROM_TRIM_OVERRIDE_COUNT;
extern const rom_write_fix_t ROM_WRITE_FIXES[];
extern const size_t ROM_WRITE_FIX_COUNT;
extern const fire_emblem_fix_t ROM_FIRE_EMBLEM_FIXES[];
extern const size_t ROM_FIRE_EMBLEM_FIX_COUNT;

/** Game that needs its IWRAM size word lowered (see patch.c). */
#define ROM_FIX_IWRAM_SIZE_GAME 0x50424732u

#endif /* PATCH_ROM_FIXES_H */
