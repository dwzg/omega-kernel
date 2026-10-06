/**
 * @file assets_gba.c
 * @brief Fonts linked into the ROM (generated from assets/fonts).
 */
#include "gfx/font.h"

#include <stddef.h>

#include "font_body_bin.h"
#include "font_small_bin.h"
#include "font_title_bin.h"

const font_t FONT_BODY = {font_body_bin, NULL};
const font_t FONT_SMALL = {font_small_bin, NULL};
const font_t FONT_TITLE = {font_title_bin, &FONT_BODY};
