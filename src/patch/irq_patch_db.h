/**
 * @file irq_patch_db.h
 * @brief Database of IRQ-vector reference locations (see src/data/irq_patch_db.c).
 */
#ifndef PATCH_IRQ_PATCH_DB_H
#define PATCH_IRQ_PATCH_DB_H

#include <stddef.h>
#include <stdint.h>

/** Terminator record value. */
#define IRQ_PATCH_DB_END 0xFFFFFFFFu

extern const uint32_t irq_patch_db[];
extern const size_t irq_patch_db_words;

#endif /* PATCH_IRQ_PATCH_DB_H */
