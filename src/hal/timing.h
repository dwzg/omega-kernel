/**
 * @file timing.h
 * @brief Busy-wait delay used by the hardware drivers.
 */
#ifndef HAL_TIMING_H
#define HAL_TIMING_H

#include <stdint.h>

/**
 * @brief Spin for @p iterations loop iterations.
 *
 * The delay counts used by the drivers were tuned on hardware with this loop
 * executing from cartridge ROM. It is intentionally *not* placed in IWRAM,
 * where it would run several times faster and shorten every delay.
 */
void delay_loop(uint32_t iterations);

#endif /* HAL_TIMING_H */
