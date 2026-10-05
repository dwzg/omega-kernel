/**
 * @file timing.c
 * @brief Busy-wait delay. See timing.h. Must stay in ROM (no IWRAM_CODE).
 */
#include "hal/timing.h"

void delay_loop(uint32_t iterations)
{
    volatile int i;

    for (i = (int)iterations; i; --i) {
    }
}
