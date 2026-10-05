/**
 * @file host.h
 * @brief Support code for running kernel modules on a PC (tests, simulator).
 */
#ifndef TESTS_HOST_H
#define TESTS_HOST_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Load the fonts for the host build.
 * @param root Repository root (fonts are read from assets/fonts/).
 * @return false (with a message on stderr) if a file is missing.
 */
bool host_assets_load(const char *root);

/** @brief Write the frame buffer to @p path as a binary PPM image. */
bool host_save_screenshot(const char *path);

/** @brief CRC-32 of the frame buffer (for comparing screens). */
uint32_t host_screen_crc(void);

#endif /* TESTS_HOST_H */
