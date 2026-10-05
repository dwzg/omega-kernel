/**
 * @file payloads.c
 * @brief Payload descriptors built from the assembly labels (GBA build only).
 */
#include "patch/payloads.h"

#include <stdint.h>

#include "pocketnes_irq_bin.h"

/* Labels exported by src/patch/payloads/ *.s. Only their addresses matter. */
extern const uint8_t Sleep_ReplaceIRQ_start[], Sleep_ReplaceIRQ_end[], Return_address_L[],
    Sleep_key[], Reset_key[];
extern const uint8_t RTS_ReplaceIRQ_start[], RTS_ReplaceIRQ_end[], RTS_Return_address_L[],
    RTS_Sleep_key[], RTS_Reset_key[], RTS_switch[], Cheat_count[], CHEAT[], no_CHEAT_end[];
extern const uint8_t RTS_only_ReplaceIRQ_start[], RTS_only_ReplaceIRQ_end[],
    RTS_only_Return_address_L[], RTS_only_SAVE_key[], RTS_only_LOAD_key[];
extern const uint8_t Fire_Emblem_0378_patch_start[], Fire_Emblem_0378_patch_end[];
extern const uint8_t Fire_Emblem_1692_patch_start[], Fire_Emblem_1692_patch_end[];
extern const uint8_t Fire_Emblem_A_patch_start[], Fire_Emblem_A_patch_end[], Modify_address_A[];
extern const uint8_t Fire_Emblem_B_patch_start[], Fire_Emblem_B_patch_end[], Modify_address_B[];
extern const uint8_t Fire_Emblem_iQue_patch_start[], Fire_Emblem_iQue_patch_end[];

/* The labels are separate symbols to C, so their distance is computed on
 * addresses rather than by (undefined) subtraction of unrelated pointers. */
#define OFFSET(label, start) ((uint32_t)((uintptr_t)(label) - (uintptr_t)(start)))
#define SIZE(start, end) OFFSET(end, start)

static struct patch_payloads payloads;
static bool initialised;

static payload_t fire_emblem(const uint8_t *start, const uint8_t *end, const uint8_t *modify)
{
    payload_t p = {
        .code = start,
        .size = SIZE(start, end),
        .return_address = PAYLOAD_NO_FIELD,
        .key_a = PAYLOAD_NO_FIELD,
        .key_b = PAYLOAD_NO_FIELD,
        .save_state_switch = PAYLOAD_NO_FIELD,
        .cheat_count = PAYLOAD_NO_FIELD,
        .cheat_table = PAYLOAD_NO_FIELD,
        .size_without_cheats = SIZE(start, end),
        .modify_address = modify ? OFFSET(modify, start) : PAYLOAD_NO_FIELD,
    };
    return p;
}

const struct patch_payloads *payloads_builtin(void)
{
    if (initialised) {
        return &payloads;
    }

    const uint8_t *s = Sleep_ReplaceIRQ_start;
    payloads.sleep = (payload_t){
        .code = s,
        .size = SIZE(s, Sleep_ReplaceIRQ_end),
        .return_address = OFFSET(Return_address_L, s),
        .key_a = OFFSET(Sleep_key, s),
        .key_b = OFFSET(Reset_key, s),
        .save_state_switch = PAYLOAD_NO_FIELD,
        .cheat_count = PAYLOAD_NO_FIELD,
        .cheat_table = PAYLOAD_NO_FIELD,
        .size_without_cheats = SIZE(s, Sleep_ReplaceIRQ_end),
        .modify_address = PAYLOAD_NO_FIELD,
    };

    s = RTS_ReplaceIRQ_start;
    payloads.full = (payload_t){
        .code = s,
        .size = SIZE(s, RTS_ReplaceIRQ_end),
        .return_address = OFFSET(RTS_Return_address_L, s),
        .key_a = OFFSET(RTS_Sleep_key, s),
        .key_b = OFFSET(RTS_Reset_key, s),
        .save_state_switch = OFFSET(RTS_switch, s),
        .cheat_count = OFFSET(Cheat_count, s),
        .cheat_table = OFFSET(CHEAT, s),
        .size_without_cheats = SIZE(s, no_CHEAT_end),
        .modify_address = PAYLOAD_NO_FIELD,
    };

    s = RTS_only_ReplaceIRQ_start;
    payloads.save_state_only = (payload_t){
        .code = s,
        .size = SIZE(s, RTS_only_ReplaceIRQ_end),
        .return_address = OFFSET(RTS_only_Return_address_L, s),
        .key_a = OFFSET(RTS_only_SAVE_key, s),
        .key_b = OFFSET(RTS_only_LOAD_key, s),
        .save_state_switch = PAYLOAD_NO_FIELD,
        .cheat_count = PAYLOAD_NO_FIELD,
        .cheat_table = PAYLOAD_NO_FIELD,
        .size_without_cheats = SIZE(s, RTS_only_ReplaceIRQ_end),
        .modify_address = PAYLOAD_NO_FIELD,
    };

    payloads.fire_emblem[FE_PAYLOAD_0378] =
        fire_emblem(Fire_Emblem_0378_patch_start, Fire_Emblem_0378_patch_end, NULL);
    payloads.fire_emblem[FE_PAYLOAD_1692] =
        fire_emblem(Fire_Emblem_1692_patch_start, Fire_Emblem_1692_patch_end, NULL);
    payloads.fire_emblem[FE_PAYLOAD_A] =
        fire_emblem(Fire_Emblem_A_patch_start, Fire_Emblem_A_patch_end, Modify_address_A);
    payloads.fire_emblem[FE_PAYLOAD_B] =
        fire_emblem(Fire_Emblem_B_patch_start, Fire_Emblem_B_patch_end, Modify_address_B);
    payloads.fire_emblem[FE_PAYLOAD_IQUE] =
        fire_emblem(Fire_Emblem_iQue_patch_start, Fire_Emblem_iQue_patch_end, NULL);

    payloads.nes_patch = pocketnes_irq_bin;
    payloads.nes_patch_size = pocketnes_irq_bin_size;

    initialised = true;
    return &payloads;
}
