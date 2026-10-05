/**
 * @file firmware.h
 * @brief Updating the cartridge's FPGA firmware.
 *
 * The kernel carries the newest FPGA configuration (assets/firmware/
 * omega_fpga.bin). At start-up it compares the running firmware version with
 * ::FIRMWARE_BUNDLED_VERSION and offers an update if it is older.
 */
#ifndef LOADER_FIRMWARE_H
#define LOADER_FIRMWARE_H

#include <stdbool.h>
#include <stdint.h>

#include "loader/progress.h"

/** Version of the bundled FPGA image. */
#define FIRMWARE_BUNDLED_VERSION 9u
/** CRC-32 of the bundled image (checked before flashing). */
#define FIRMWARE_BUNDLED_CRC32 0xB23F6EAEu
/** Version reported by test firmware builds; always offered an update. */
#define FIRMWARE_TEST_VERSION 99u

/** @brief true if a cartridge running @p version should be updated. */
bool firmware_update_available(uint16_t version);

/** @brief true if the bundled image is intact. */
bool firmware_image_valid(void);

/**
 * @brief Write the bundled image to the FPGA configuration flash.
 * The new firmware is active after the next power cycle.
 */
void firmware_write(const progress_t *progress);

#endif /* LOADER_FIRMWARE_H */
