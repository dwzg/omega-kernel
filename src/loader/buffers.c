/**
 * @file buffers.c
 * @brief Shared EWRAM buffers. See buffers.h.
 */
#include "loader/buffers.h"

#include "platform/attributes.h"

uint8_t g_scratch[SCRATCH_SIZE] PLATFORM_EWRAM __attribute__((aligned(4)));
cheat_code_t g_cheat_codes[MAX_CHEAT_CODES] PLATFORM_EWRAM;
uint32_t g_cheat_code_count;
cht_workspace_t g_cheat_workspace PLATFORM_EWRAM;
recent_list_t g_recent PLATFORM_EWRAM;
