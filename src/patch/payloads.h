/**
 * @file payloads.h
 * @brief Hook payloads: assembly code that is copied into patched games.
 *
 * Each payload is position-independent ARM/Thumb code assembled into the
 * kernel (src/patch/payloads/ *.s). The patch engine copies a payload to a
 * work buffer, fills in its parameter words (return address, hotkey masks,
 * cheat table, ...) and writes it into the game at the trim size.
 *
 * The offsets below locate those parameter words relative to the start of
 * the payload. On the GBA they come from the assembly labels (payloads.c);
 * the host tests provide fake payloads with the same layout.
 */
#ifndef PATCH_PAYLOADS_H
#define PATCH_PAYLOADS_H

#include <stdbool.h>
#include <stdint.h>

/** Marker for "this payload has no such parameter". */
#define PAYLOAD_NO_FIELD 0xFFFFFFFFu

/** A payload's code and the offsets of its parameter words. */
typedef struct {
    const uint8_t *code;          /**< Start of the payload. */
    uint32_t size;                /**< Bytes to copy into the game. */
    uint32_t return_address;      /**< Word: game entry point to continue at. */
    uint32_t key_a;               /**< Word: first hotkey mask (sleep, or save state). */
    uint32_t key_b;               /**< Word: second hotkey mask (menu, or load state). */
    uint32_t save_state_switch;   /**< Word: save states on/off (full payload only). */
    uint32_t cheat_count;         /**< Word: number of cheat codes (full payload only). */
    uint32_t cheat_table;         /**< Start of the cheat table (full payload only). */
    uint32_t size_without_cheats; /**< Bytes before the cheat table. */
    uint32_t modify_address;      /**< Word: game-specific address (Fire Emblem). */
} payload_t;

/** The Fire Emblem save-fix payload variants. */
typedef enum {
    FE_PAYLOAD_0378, /**< Fuuin no Tsurugi (JP) */
    FE_PAYLOAD_1692, /**< Seima no Kouseki (JP) */
    FE_PAYLOAD_A,    /**< Rekka no Ken / Fire Emblem (US/EU) */
    FE_PAYLOAD_B,    /**< The Sacred Stones (US/EU) */
    FE_PAYLOAD_IQUE, /**< iQue prototype */
    FE_PAYLOAD_COUNT
} fe_payload_id_t;

/** All payloads the patch engine can use. */
struct patch_payloads {
    payload_t sleep;           /**< Reset-to-menu and sleep hooks. */
    payload_t full;            /**< Save states, cheats, reset and sleep. */
    payload_t save_state_only; /**< Save states only (smaller, separate keys). */
    payload_t fire_emblem[FE_PAYLOAD_COUNT];
    const uint8_t *nes_patch; /**< PocketNES IRQ fix, @ref nes_patch_size bytes. */
    uint32_t nes_patch_size;
};

/** @brief The payloads linked into the kernel. */
const struct patch_payloads *payloads_builtin(void);

#endif /* PATCH_PAYLOADS_H */
