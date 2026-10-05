/**
 * @file datetime.h
 * @brief Calendar helpers and conversion of the RTC's BCD registers.
 *
 * Portable: no GBA dependencies, unit-tested on the host.
 */
#ifndef CORE_DATETIME_H
#define CORE_DATETIME_H

#include <stdint.h>

/** A calendar date and time of day in plain binary. */
typedef struct {
    uint8_t year;    /**< 0-99, meaning 2000-2099 */
    uint8_t month;   /**< 1-12 */
    uint8_t day;     /**< 1-31 */
    uint8_t weekday; /**< 0 = Sunday ... 6 = Saturday */
    uint8_t hour;    /**< 0-23 */
    uint8_t minute;  /**< 0-59 */
    uint8_t second;  /**< 0-59 */
} datetime_t;

/** Field order of the 7-byte BCD record used by the RTC. */
enum {
    DATETIME_YEAR = 0,
    DATETIME_MONTH,
    DATETIME_DAY,
    DATETIME_WEEKDAY,
    DATETIME_HOUR,
    DATETIME_MINUTE,
    DATETIME_SECOND,
    DATETIME_FIELD_COUNT
};

/** @brief Convert a packed BCD byte (e.g. 0x59) to binary (59). */
uint8_t bcd_to_bin(uint8_t bcd);

/** @brief Convert a binary value 0-99 to packed BCD. */
uint8_t bin_to_bcd(uint8_t value);

/** @brief Days in @p month (1-12) of year 2000 + @p year. */
uint8_t days_in_month(uint8_t month, uint8_t year);

/**
 * @brief Decode a 7-byte RTC record.
 *
 * Status bits that share the registers (e.g. the PM flag in the hour) are
 * masked off and any out-of-range field is replaced by its minimum, so the
 * result is always a valid date even if the clock was never set.
 */
void datetime_from_bcd(datetime_t *dt, const uint8_t bcd[DATETIME_FIELD_COUNT]);

/** @brief Encode @p dt into the RTC's 7-byte BCD record. */
void datetime_to_bcd(const datetime_t *dt, uint8_t bcd[DATETIME_FIELD_COUNT]);

/**
 * @brief Decode the RTC's 3-byte time-of-day record (hour, minute, second).
 * Fields of @p dt other than the time are left untouched.
 */
void datetime_time_from_bcd(datetime_t *dt, const uint8_t bcd[3]);

/** @brief Pack @p dt into a FAT file system timestamp. */
uint32_t datetime_to_fat(const datetime_t *dt);

/** @brief Force every field of @p dt into its valid range. */
void datetime_sanitize(datetime_t *dt);

/** @brief Day of the week (0 = Sunday) of the date in @p dt (2000-2099). */
uint8_t datetime_weekday(const datetime_t *dt);

/** @brief Three-letter English weekday name ("Sun" ... "Sat"). */
const char *datetime_weekday_name(uint8_t weekday);

#endif /* CORE_DATETIME_H */
