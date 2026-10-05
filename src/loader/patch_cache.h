/**
 * @file patch_cache.h
 * @brief `.pat` files: cached patch-engine results in /PATCH.
 *
 * Scanning a ROM for IRQ references takes a while, so the result is stored
 * per game and reused as long as the hook settings have not changed.
 */
#ifndef LOADER_PATCH_CACHE_H
#define LOADER_PATCH_CACHE_H

#include <stdbool.h>

#include "patch/patch.h"

/** @brief Load the cache for @p game_filename into @p ctx. @return true if it was usable. */
bool patch_cache_load(patch_context_t *ctx, const char *game_filename);

/** @brief Store the state of @p ctx as the cache for @p game_filename. */
bool patch_cache_store(const patch_context_t *ctx, const char *game_filename);

#endif /* LOADER_PATCH_CACHE_H */
