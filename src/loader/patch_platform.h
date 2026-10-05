/**
 * @file patch_platform.h
 * @brief The patch engine's hardware services on the GBA.
 */
#ifndef LOADER_PATCH_PLATFORM_H
#define LOADER_PATCH_PLATFORM_H

#include "patch/patch.h"

/** @brief PSRAM writer and VRAM work buffer for the patch engine. */
const patch_platform_t *patch_platform_gba(void);

#endif /* LOADER_PATCH_PLATFORM_H */
