/**
 * @file reset.h
 * @brief Assembly routines that leave the kernel (see reset.s).
 *
 * The routines live in IWRAM and are Thumb code. They are declared without
 * IWRAM_CODE on purpose: that macro adds long_call, and the call must stay a
 * plain BL from omega_boot() (also in IWRAM) - by the time they run, the ROM
 * page has already been switched to the game.
 */
#ifndef HAL_RESET_H
#define HAL_RESET_H

#include <gba_base.h>

/** @brief Wipe IWRAM and restart at 0x08000000 through BIOS SoftReset. */
void reset_soft(void) __attribute__((noreturn));

/** @brief Restart through the BIOS boot sequence (BIOS HardReset). */
void reset_hard(void) __attribute__((noreturn));

/** @brief BIOS RegisterRamReset (SWI 0x01) callable from IWRAM. */
void reset_register_ram(u32 flags);

#endif /* HAL_RESET_H */
