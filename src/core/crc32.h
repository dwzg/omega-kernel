/**
 * @file crc32.h
 * @brief CRC-32 (IEEE 802.3, as used by zlib/PNG).
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_CRC32_H
#define CORE_CRC32_H

#include <stddef.h>
#include <stdint.h>

/** @brief CRC-32 of @p size bytes at @p data. */
uint32_t crc32(const void *data, size_t size);

#endif /* CORE_CRC32_H */
