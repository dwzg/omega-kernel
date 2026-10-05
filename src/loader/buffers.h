/**
 * @file buffers.h
 * @brief Large work buffers shared by the loader and the UI (in EWRAM).
 *
 * EWRAM is 256 KiB and almost fully used, so the biggest buffers are shared.
 * Rules for ::g_scratch:
 *  - Nobody may expect its contents to survive a call into another module.
 *  - It is 32-bit aligned and may be used with DMA.
 */
#ifndef LOADER_BUFFERS_H
#define LOADER_BUFFERS_H

#include <stdint.h>

#include "core/cheat.h"
#include "core/recent.h"

/** Size of ::g_scratch: one NOR block / one ROM read chunk. */
#define SCRATCH_SIZE 0x20000u

/** Maximum number of active cheat codes. */
#define MAX_CHEAT_CODES 3000

/** General-purpose 128 KiB buffer (ROM blocks, thumbnails, cheat menu, ...). */
extern uint8_t g_scratch[SCRATCH_SIZE];

/** Cheat codes chosen for the game about to start. */
extern cheat_code_t g_cheat_codes[MAX_CHEAT_CODES];
/** Number of valid entries in ::g_cheat_codes. */
extern uint32_t g_cheat_code_count;

/** Work memory of the cheat file parser. */
extern cht_workspace_t g_cheat_workspace;

/**
 * The recently played list. Shared by the "Recently Played" screen and the
 * boot code (which records the game being started); both load it from the
 * SD card before use.
 */
extern recent_list_t g_recent;

#endif /* LOADER_BUFFERS_H */
