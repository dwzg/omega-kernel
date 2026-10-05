/**
 * @file datetime.c
 * @brief Calendar helpers. See datetime.h.
 */
#include "core/datetime.h"

uint8_t bcd_to_bin(uint8_t bcd)
{
    return (uint8_t)((bcd & 0x0F) + (bcd >> 4) * 10);
}

uint8_t bin_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10) << 4) + (value % 10));
}

uint8_t days_in_month(uint8_t month, uint8_t year)
{
    switch (month) {
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    case 2:
        /* Every year divisible by 4 in 2000-2099 is a leap year. */
        return (year % 4 == 0) ? 29 : 28;
    default:
        return 31;
    }
}

void datetime_sanitize(datetime_t *dt)
{
    if (dt->year > 99) {
        dt->year = 0;
    }
    if (dt->month < 1 || dt->month > 12) {
        dt->month = 1;
    }
    if (dt->day < 1 || dt->day > days_in_month(dt->month, dt->year)) {
        dt->day = 1;
    }
    if (dt->weekday > 6) {
        dt->weekday = 0;
    }
    if (dt->hour > 23) {
        dt->hour = 0;
    }
    if (dt->minute > 59) {
        dt->minute = 0;
    }
    if (dt->second > 59) {
        dt->second = 0;
    }
}

void datetime_time_from_bcd(datetime_t *dt, const uint8_t bcd[3])
{
    dt->hour = bcd_to_bin(bcd[0] & 0x3F);
    dt->minute = bcd_to_bin(bcd[1] & 0x7F);
    dt->second = bcd_to_bin(bcd[2] & 0x7F);
    if (dt->hour > 23) {
        dt->hour = 0;
    }
    if (dt->minute > 59) {
        dt->minute = 0;
    }
    if (dt->second > 59) {
        dt->second = 0;
    }
}

void datetime_from_bcd(datetime_t *dt, const uint8_t bcd[DATETIME_FIELD_COUNT])
{
    dt->year = bcd_to_bin(bcd[DATETIME_YEAR]);
    dt->month = bcd_to_bin(bcd[DATETIME_MONTH] & 0x1F);
    dt->day = bcd_to_bin(bcd[DATETIME_DAY] & 0x3F);
    dt->weekday = bcd_to_bin(bcd[DATETIME_WEEKDAY] & 0x07);
    datetime_time_from_bcd(dt, &bcd[DATETIME_HOUR]);
    datetime_sanitize(dt);
}

void datetime_to_bcd(const datetime_t *dt, uint8_t bcd[DATETIME_FIELD_COUNT])
{
    bcd[DATETIME_YEAR] = bin_to_bcd(dt->year);
    bcd[DATETIME_MONTH] = bin_to_bcd(dt->month);
    bcd[DATETIME_DAY] = bin_to_bcd(dt->day);
    bcd[DATETIME_WEEKDAY] = bin_to_bcd(dt->weekday);
    bcd[DATETIME_HOUR] = bin_to_bcd(dt->hour);
    bcd[DATETIME_MINUTE] = bin_to_bcd(dt->minute);
    bcd[DATETIME_SECOND] = bin_to_bcd(dt->second);
}

uint32_t datetime_to_fat(const datetime_t *dt)
{
    /* FAT years count from 1980; RTC years from 2000. */
    return ((uint32_t)(dt->year + 20) << 25) | ((uint32_t)dt->month << 21) |
           ((uint32_t)dt->day << 16) | ((uint32_t)dt->hour << 11) | ((uint32_t)dt->minute << 5) |
           ((uint32_t)dt->second >> 1);
}

const char *datetime_weekday_name(uint8_t weekday)
{
    static const char *const names[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return names[weekday < 7 ? weekday : 0];
}

uint8_t datetime_weekday(const datetime_t *dt)
{
    /* Tomohiko Sakamoto's algorithm. */
    static const uint8_t OFFSETS[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int year = 2000 + dt->year;
    int month = (dt->month >= 1 && dt->month <= 12) ? dt->month : 1;
    if (month < 3) {
        year -= 1;
    }
    return (uint8_t)((year + year / 4 - year / 100 + year / 400 + OFFSETS[month - 1] + dt->day) %
                     7);
}
