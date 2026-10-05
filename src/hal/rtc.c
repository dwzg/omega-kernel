/**
 * @file rtc.c
 * @brief S-3511 RTC driver (serial protocol bit-banged over GPIO). See rtc.h.
 *
 * GPIO pins (register bits): 0 = SCK, 1 = SIO, 2 = CS.
 */
#include "hal/rtc.h"

#include "hal/timing.h"

#define GPIO_DATA ((vu16 *)0x080000C4)      /**< pin levels */
#define GPIO_DIRECTION ((vu16 *)0x080000C6) /**< 1 = output, per pin */
#define GPIO_ENABLE ((vu16 *)0x080000C8)    /**< 1 = GPIO visible to reads */

#define CMD_READ(reg) (((reg) << 1) | 0x61)
#define CMD_WRITE(reg) (((reg) << 1) | 0x60)
#define REG_DATETIME 2
#define REG_TIME 3

/** Clock out a command byte, most significant bit first. */
static void send_command(int value)
{
    value <<= 1;
    for (int bit = 7; bit >= 0; bit--) {
        u16 sio = (u16)((value >> bit) & 0x2);
        *GPIO_DATA = sio | 4;
        *GPIO_DATA = sio | 4;
        *GPIO_DATA = sio | 4;
        *GPIO_DATA = sio | 5;
    }
}

/** Clock out a data byte, least significant bit first. */
static void send_byte(int value)
{
    value <<= 1;
    for (int bit = 0; bit < 8; bit++) {
        u16 sio = (u16)((value >> bit) & 0x2);
        *GPIO_DATA = sio | 4;
        *GPIO_DATA = sio | 4;
        *GPIO_DATA = sio | 4;
        *GPIO_DATA = sio | 5;
    }
}

/** Clock in a data byte, least significant bit first. */
static u8 receive_byte(void)
{
    int value = 0;
    for (int bit = 0; bit < 8; bit++) {
        for (int i = 0; i < 5; i++) {
            *GPIO_DATA = 4;
        }
        *GPIO_DATA = 5;
        u16 pins = *GPIO_DATA;
        value |= (pins & 2) << bit;
    }
    return (u8)(value >> 1);
}

static void begin_read(int reg)
{
    *GPIO_DATA = 1;
    *GPIO_DIRECTION = 7;
    *GPIO_DATA = 1;
    *GPIO_DATA = 5;
    send_command(CMD_READ(reg));
    *GPIO_DIRECTION = 5;
}

void rtc_read_datetime(u8 bcd[RTC_DATETIME_BYTES])
{
    *GPIO_ENABLE = 1;
    begin_read(REG_DATETIME);
    for (int i = 0; i < 4; i++) {
        bcd[i] = receive_byte();
    }
    *GPIO_DIRECTION = 5;
    for (int i = 4; i < 7; i++) {
        bcd[i] = receive_byte();
    }
    *GPIO_ENABLE = 0;
}

void rtc_read_time(u8 bcd[RTC_TIME_BYTES])
{
    *GPIO_ENABLE = 1;
    begin_read(REG_TIME);
    for (int i = 0; i < 3; i++) {
        bcd[i] = receive_byte();
    }
    *GPIO_ENABLE = 0;
    delay_loop(5);
}

void rtc_write_datetime(const u8 bcd[RTC_DATETIME_BYTES])
{
    *GPIO_ENABLE = 1;
    *GPIO_DATA = 1;
    *GPIO_DATA = 5;
    *GPIO_DIRECTION = 7;
    send_command(CMD_WRITE(REG_DATETIME));
    for (int i = 0; i < 7; i++) {
        send_byte(bcd[i]);
    }
    *GPIO_ENABLE = 0;
    delay_loop(0x200);
}
