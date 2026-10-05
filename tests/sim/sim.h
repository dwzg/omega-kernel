/**
 * @file sim.h
 * @brief The UI simulator: runs the real interface code on a PC.
 *
 * The kernel's UI, loader file code and FatFs are compiled for the host.
 * Hardware is replaced by:
 *  - platform_sim.c: frame buffer, scripted buttons, a fixed clock and
 *    settings kept in memory;
 *  - disk_sim.c: an SD card image file (built by sim.mk with mtools);
 *  - stubs.c: game start, NOR flash and firmware update.
 *
 * Scenarios (scenarios.c) press buttons and take screenshots; each
 * screenshot's CRC is compared with tests/sim/golden.txt.
 */
#ifndef TESTS_SIM_H
#define TESTS_SIM_H

#include <stdbool.h>
#include <stdint.h>

#include "loader/nor_games.h"
#include "platform/platform.h"

/** One step of a scenario script. */
typedef struct {
    enum { STEP_PRESS, STEP_HOLD, STEP_RELEASE, STEP_WAIT, STEP_SHOT, STEP_END } kind;
    uint16_t buttons; /**< STEP_PRESS / STEP_HOLD / STEP_RELEASE */
    unsigned frames;  /**< STEP_WAIT */
    const char *name; /**< STEP_SHOT */
} sim_step_t;

#define PRESS(b)                                                                                   \
    {                                                                                              \
        STEP_PRESS, (b), 0, NULL                                                                   \
    }
#define HOLD(b)                                                                                    \
    {                                                                                              \
        STEP_HOLD, (b), 0, NULL                                                                    \
    }
#define RELEASE(b)                                                                                 \
    {                                                                                              \
        STEP_RELEASE, (b), 0, NULL                                                                 \
    }
#define WAIT(n)                                                                                    \
    {                                                                                              \
        STEP_WAIT, 0, (n), NULL                                                                    \
    }
#define SHOT(name)                                                                                 \
    {                                                                                              \
        STEP_SHOT, 0, 0, (name)                                                                    \
    }
#define END                                                                                        \
    {                                                                                              \
        STEP_END, 0, 0, NULL                                                                       \
    }

/** A screenshot taken by a scenario. */
typedef void (*sim_shot_fn)(const char *name);

/**
 * @brief Run the interface with @p script until the script ends.
 * @param settings_words Initial settings block (NULL = erased flash).
 * @param shot           Called for every SHOT step.
 */
void sim_run(const sim_step_t *script, const uint16_t *settings_words, sim_shot_fn shot);

/** @brief Mount the SD image at @p path (fails the program if impossible). */
void sim_disk_open(const char *path);

/** @brief Fill the simulated NOR library with @p count example games. */
void sim_nor_set_games(unsigned count);

/** @brief Last value written by platform_settings_store(), or NULL. */
const uint16_t *sim_stored_settings(void);

/** @brief Number of times a game start was requested. */
unsigned sim_boot_requests(void);

#endif /* TESTS_SIM_H */
