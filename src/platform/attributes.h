/**
 * @file attributes.h
 * @brief Memory placement attributes that compile away on the host.
 *
 * - ::PLATFORM_EWRAM puts a large zero-initialised object in the 256 KiB
 *   external work RAM instead of the 32 KiB internal RAM.
 * - ::PLATFORM_FAST_CODE places a function in internal RAM, where it runs
 *   faster (and keeps running while cartridge ROM is unavailable).
 */
#ifndef PLATFORM_ATTRIBUTES_H
#define PLATFORM_ATTRIBUTES_H

#ifdef __GBA__
#include <gba_base.h>
#define PLATFORM_EWRAM EWRAM_BSS
#define PLATFORM_FAST_CODE IWRAM_CODE
#else
#define PLATFORM_EWRAM
#define PLATFORM_FAST_CODE
#endif

#endif /* PLATFORM_ATTRIBUTES_H */
