/**
 * @file rtc.h
 * @brief Seiko S-3511 real-time clock on the cartridge GPIO port.
 *
 * Values are exchanged in the chip's raw BCD format; use core/datetime.h to
 * convert and validate them.
 *
 * Raw date/time layout (7 bytes): year, month, day, weekday, hour, minute,
 * second. The time-only read returns the last three.
 */
#ifndef HAL_RTC_H
#define HAL_RTC_H

#include <gba_base.h>

/** Number of bytes in a raw date/time record. */
#define RTC_DATETIME_BYTES 7
/** Number of bytes in a raw time record. */
#define RTC_TIME_BYTES 3

/** @brief Read the full date and time (raw BCD). */
void rtc_read_datetime(u8 bcd[RTC_DATETIME_BYTES]);

/** @brief Read the time of day only (raw BCD hour, minute, second). */
void rtc_read_time(u8 bcd[RTC_TIME_BYTES]);

/** @brief Set the clock (raw BCD, same layout as rtc_read_datetime()). */
void rtc_write_datetime(const u8 bcd[RTC_DATETIME_BYTES]);

#endif /* HAL_RTC_H */
