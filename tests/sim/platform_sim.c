/**
 * @file platform_sim.c
 * @brief platform.h for the simulator: scripted input, fixed clock.
 */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader/sd_paths.h"
#include "sim.h"
#include "ui/app.h"

/** Frames after which an unfinished script is considered stuck. */
#define MAX_FRAMES 100000

static const sim_step_t *s_step;
static sim_shot_fn s_shot;
static jmp_buf s_end;
static uint16_t s_held;
static uint16_t s_previous;
static uint16_t s_tap;
static unsigned s_wait;
static unsigned s_frames;
static datetime_t s_clock;
static uint16_t s_settings[SETTINGS_WORDS];
static uint16_t s_stored[SETTINGS_WORDS];
static bool s_stored_valid;
static app_t s_app;

void platform_init(void)
{
}

void platform_wait_vblank(void)
{
    if (++s_frames > MAX_FRAMES) {
        fprintf(stderr, "sim: script did not finish (UI stuck waiting?)\n");
        exit(1);
    }
}

/** Advance the script to the input of the next frame. */
static uint16_t next_buttons(void)
{
    if (s_tap) {
        /* Second frame of a PRESS: release the buttons again. */
        s_held &= (uint16_t)~s_tap;
        s_tap = 0;
        return s_held;
    }
    if (s_wait > 0) {
        s_wait--;
        return s_held;
    }
    for (;;) {
        const sim_step_t *step = s_step++;
        switch (step->kind) {
        case STEP_PRESS:
            /* Down for one frame, up the next. */
            s_held |= step->buttons;
            s_tap = step->buttons;
            return s_held;
        case STEP_HOLD:
            s_held |= step->buttons;
            break;
        case STEP_RELEASE:
            s_held &= (uint16_t)~step->buttons;
            break;
        case STEP_WAIT:
            if (step->frames > 0) {
                s_wait = step->frames - 1;
                return s_held;
            }
            break;
        case STEP_SHOT:
            s_shot(step->name);
            break;
        case STEP_END:
            longjmp(s_end, 1);
        }
    }
}

void platform_read_input(input_t *input)
{
    uint16_t now = next_buttons();
    input->pressed = now & (uint16_t)~s_previous;
    input->released = s_previous & (uint16_t)~now;
    input->held = now;
    input->repeated = input->pressed;
    s_previous = now;
}

void platform_set_key_repeat(unsigned delay, unsigned rate)
{
}

void platform_clock_read(datetime_t *now)
{
    *now = s_clock;
}

void platform_clock_write(const datetime_t *now)
{
    s_clock = *now;
}

void platform_settings_load(uint16_t words[SETTINGS_WORDS])
{
    memcpy(words, s_settings, sizeof(s_settings));
}

void platform_settings_store(const uint16_t words[SETTINGS_WORDS])
{
    memcpy(s_settings, words, sizeof(s_settings));
    memcpy(s_stored, words, sizeof(s_stored));
    s_stored_valid = true;
}

uint16_t platform_fpga_version(void)
{
    return 9;
}

const uint16_t *sim_stored_settings(void)
{
    return s_stored_valid ? s_stored : NULL;
}

void sim_run(const sim_step_t *script, const uint16_t *settings_words, sim_shot_fn shot)
{
    s_step = script;
    s_shot = shot;
    s_held = s_previous = s_tap = 0;
    s_wait = 0;
    s_frames = 0;
    s_stored_valid = false;
    s_clock =
        (datetime_t){.year = 26, .month = 10, .day = 5, .weekday = 1, .hour = 9, .minute = 41};
    if (settings_words) {
        memcpy(s_settings, settings_words, sizeof(s_settings));
    } else {
        memset(s_settings, 0xFF, sizeof(s_settings));
    }

    if (setjmp(s_end) == 0) {
        memset(&s_app, 0, sizeof(s_app));
        s_app.fpga_version = platform_fpga_version();
        app_load_settings(&s_app);
        ui_run(&s_app);
    }
}
