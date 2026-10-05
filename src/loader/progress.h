/**
 * @file progress.h
 * @brief How long-running loader operations report what they are doing.
 *
 * The loader does not draw anything itself; the UI passes a ::progress_t
 * whose callbacks update the screen.
 */
#ifndef LOADER_PROGRESS_H
#define LOADER_PROGRESS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    /** Show a short description of the current step ("Creating save file"). */
    void (*status)(void *ctx, const char *text);
    /** Report @p done of @p total units of work. */
    void (*advance)(void *ctx, uint32_t done, uint32_t total);
    void *ctx;
} progress_t;

/** @brief Report a step, if @p p is not NULL. */
static inline void progress_status(const progress_t *p, const char *text)
{
    if (p && p->status) {
        p->status(p->ctx, text);
    }
}

/** @brief Report progress, if @p p is not NULL. */
static inline void progress_advance(const progress_t *p, uint32_t done, uint32_t total)
{
    if (p && p->advance) {
        p->advance(p->ctx, done, total);
    }
}

#endif /* LOADER_PROGRESS_H */
