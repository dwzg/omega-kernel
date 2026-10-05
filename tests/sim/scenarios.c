/**
 * @file scenarios.c
 * @brief UI simulator: scripted walks through every screen, screenshot
 * comparison against golden.txt, and file-listing regression checks.
 *
 * Usage: sim <sd.img> <golden.txt> [--update] [--screens DIR]
 *   --update       rewrite golden.txt from the current screens
 *   --screens DIR  write every screenshot to DIR as a PPM file
 * Screens that differ from golden.txt are written to build/screens-failed/.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "host.h"
#include "loader/directory.h"
#include "loader/sd_paths.h"
#include "sim.h"

/* ------------------------------------------------------------- scripts -- */

#define DOWN PRESS(BTN_DOWN)
#define UP PRESS(BTN_UP)
#define A PRESS(BTN_A)
#define B PRESS(BTN_B)

/* Root of the SD card, sorted: ATTR CHEAT GBA IMGS SAVER, then games:
 * 5 Advance Wars, 6 Castlevania, 7 Golden Sun, 8 Homebrew Demo,
 * 9 Mario & Luigi, 10 Metroid Fusion, 11 Pokemon, 12 Zelda. */

static const sim_step_t BROWSE[] = {
    WAIT(2),
    SHOT("sd_root"),
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    WAIT(2),
    SHOT("sd_root_long_name"),
    WAIT(150),
    SHOT("sd_root_marquee"),
    PRESS(BTN_L),
    PRESS(BTN_L),
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("sd_folder"), /* GBA */
    A,
    WAIT(2),
    SHOT("sd_subfolder"), /* GBA/Puzzle */
    B,
    B,
    WAIT(2),
    SHOT("sd_back_to_root"),
    B,
    WAIT(2),
    SHOT("main_menu"),
    END,
};

/* Scroll down row by row (fast path: rows are moved, not redrawn), then
 * leave and re-enter so the same position is drawn from scratch. */
static const sim_step_t SCROLL_CONSISTENCY[] = {
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN, /* Pokemon */
    WAIT(2),
    SHOT("scrolled_down"),
    UP,
    UP,
    UP,
    UP,
    UP,
    UP,
    UP,
    UP,
    UP,
    UP,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    WAIT(2),
    SHOT("scrolled_back"),
    A,
    WAIT(2),
    B,
    WAIT(2),
    SHOT("scrolled_redrawn"),
    END,
};

static const sim_step_t ATTRIBUTES[] = {
    A,
    WAIT(2),
    SHOT("sd_attributes"), /* ATTR is the first folder */
    END,
};

static const sim_step_t GAME_PAGE[] = {
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN, /* Metroid Fusion */
    A,
    WAIT(2),
    SHOT("game_page"),
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    PRESS(BTN_RIGHT),
    PRESS(BTN_RIGHT),
    WAIT(2),
    SHOT("game_page_save_type"),
    DOWN,
    A,
    WAIT(2),
    SHOT("confirm_delete"),
    B,
    WAIT(2),
    UP,
    UP,
    UP,
    UP,
    UP,
    A,
    WAIT(2),
    SHOT("boot_error"),
    A,
    B,
    WAIT(2),
    SHOT("back_to_list"),
    END,
};

static const sim_step_t HOMEBREW[] = {
    DOWN, DOWN,    DOWN,
    DOWN, DOWN,    DOWN,
    DOWN, DOWN, /* Homebrew Demo */
    A,    WAIT(2), SHOT("game_page_homebrew"),
    END,
};

static const sim_step_t CHEATS[] = {
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN, /* Golden Sun */
    A,
    WAIT(2),
    SHOT("game_page_cheats"),
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("cheats"),
    A,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("cheats_selected"),
    B,
    WAIT(2),
    SHOT("game_page_cheats_on"),
    END,
};

static const sim_step_t RECENT[] = {
    PRESS(BTN_START),
    WAIT(2),
    SHOT("recent"),
    B,
    B,
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("recent_from_menu"),
    A,
    WAIT(2),
    SHOT("recent_game_page"),
    END,
};

static const sim_step_t NOR_LIBRARY[] = {
    B,
    DOWN,
    A,
    WAIT(2),
    SHOT("nor_library"),
    A,
    WAIT(2),
    SHOT("nor_game_page"),
    DOWN,
    A,
    WAIT(2),
    SHOT("nor_delete_refused"),
    A,
    B,
    DOWN,
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("nor_last_game"),
    DOWN,
    A,
    WAIT(2),
    SHOT("nor_confirm_delete"),
    DOWN,
    A,
    WAIT(2),
    SHOT("nor_after_delete"),
    DOWN,
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("nor_confirm_erase"),
    DOWN,
    A,
    WAIT(2),
    SHOT("nor_erased"),
    A,
    WAIT(2),
    SHOT("nor_empty"),
    END,
};

static const sim_step_t SETTINGS[] = {
    B,
    DOWN,
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("settings"),
    A,
    DOWN,
    A,
    WAIT(2),
    SHOT("settings_changed"),
    DOWN,
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("hotkey_editor"),
    PRESS(BTN_RIGHT),
    PRESS(BTN_RIGHT),
    PRESS(BTN_UP),
    WAIT(2),
    SHOT("hotkey_editor_changed"),
    A,
    WAIT(2),
    SHOT("settings_hotkey_saved"),
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("datetime_editor"),
    PRESS(BTN_UP),
    PRESS(BTN_RIGHT),
    PRESS(BTN_DOWN),
    WAIT(2),
    SHOT("datetime_editor_changed"),
    A,
    WAIT(2),
    SHOT("settings_after_datetime"),
    B,
    WAIT(2),
    END,
};

static const sim_step_t ABOUT[] = {
    B, DOWN, DOWN, DOWN, DOWN, A, WAIT(2), SHOT("about"), END,
};

/* ------------------------------------------------------------- golden -- */

#define MAX_SHOTS 128

typedef struct {
    char name[48];
    uint32_t crc;
} golden_t;

static golden_t s_golden[MAX_SHOTS];
static unsigned s_golden_count;
static golden_t s_actual[MAX_SHOTS];
static unsigned s_actual_count;
static const char *s_screens_dir;

/** Where screens that differ from golden.txt are written. */
#define FAILED_SCREENS_DIR "build/screens-failed"
static unsigned s_failures;

static void load_golden(const char *path)
{
    FILE *f = fopen(path, "r");
    char name[48];
    unsigned crc;
    if (!f) {
        return;
    }
    while (s_golden_count < MAX_SHOTS && fscanf(f, "%47s %x", name, &crc) == 2) {
        snprintf(s_golden[s_golden_count].name, sizeof(name), "%s", name);
        s_golden[s_golden_count++].crc = crc;
    }
    fclose(f);
}

static void write_ppm(const char *dir, const char *name)
{
    char path[512];
    mkdir(dir, 0755);
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
    if (!host_save_screenshot(path)) {
        fprintf(stderr, "sim: cannot write %s\n", path);
    }
}

static const golden_t *find_golden(const char *name)
{
    for (unsigned k = 0; k < s_golden_count; k++) {
        if (strcmp(s_golden[k].name, name) == 0) {
            return &s_golden[k];
        }
    }
    return NULL;
}

static bool s_update;

static const golden_t *find_actual(const char *name)
{
    for (unsigned k = 0; k < s_actual_count; k++) {
        if (strcmp(s_actual[k].name, name) == 0) {
            return &s_actual[k];
        }
    }
    return NULL;
}

static void on_shot(const char *name)
{
    uint32_t crc = host_screen_crc();
    if (s_actual_count >= MAX_SHOTS) {
        fprintf(stderr, "sim: too many screenshots\n");
        exit(1);
    }
    for (unsigned i = 0; i < s_actual_count; i++) {
        if (strcmp(s_actual[i].name, name) == 0) {
            fprintf(stderr, "sim: duplicate screenshot name %s\n", name);
            exit(1);
        }
    }
    snprintf(s_actual[s_actual_count].name, sizeof(s_actual[0].name), "%s", name);
    s_actual[s_actual_count++].crc = crc;
    if (s_screens_dir) {
        write_ppm(s_screens_dir, name);
    }
    const golden_t *g = find_golden(name);
    if (!s_update && (!g || g->crc != crc)) {
        write_ppm(FAILED_SCREENS_DIR, name);
    }
}

/* ------------------------------------------------------------- checks -- */

#define EXPECT(cond)                                                                               \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond);               \
            s_failures++;                                                                          \
        }                                                                                          \
    } while (0)

static const char *s_image;

static void fresh_card(void)
{
    sim_disk_open(s_image);
    if (!sd_mount()) {
        fprintf(stderr, "sim: cannot mount %s\n", s_image);
        exit(1);
    }
}

/**
 * Regression: the original kernel listed a file only if its attribute byte
 * was exactly ARCHIVE (0x20), so games copied without the archive bit, or
 * marked read-only, were invisible.
 */
static void check_attribute_filter(void)
{
    dir_listing_t listing;
    fresh_card();
    EXPECT(directory_read("/ATTR", &listing));
    EXPECT(listing.folders == 0);
    EXPECT(listing.files == 3);
    if (listing.files == 3) {
        EXPECT(strcmp(directory_entry(0)->name, "No Archive Bit.gba") == 0);
        EXPECT(strcmp(directory_entry(1)->name, "Normal.gba") == 0);
        EXPECT(strcmp(directory_entry(2)->name, "Read Only.gba") == 0);
    }
}

/** Root listing: folders first, then games, both sorted; other files hidden. */
static void check_root_listing(void)
{
    dir_listing_t listing;
    fresh_card();
    EXPECT(directory_read("/", &listing));
    EXPECT(listing.folders == 5);
    EXPECT(listing.files == 8);
    EXPECT(strcmp(directory_entry(0)->name, "ATTR") == 0);
    EXPECT(strcmp(directory_entry(5)->name, "Advance Wars.gba") == 0);
    EXPECT(directory_entry(5)->size == 4u << 20);
    EXPECT(directory_is_folder(4) && !directory_is_folder(5));
    EXPECT(!directory_read("/missing", &listing));
}

/* -------------------------------------------------------------- main -- */

static void run(const sim_step_t *script, const uint16_t *settings)
{
    fresh_card();
    sim_nor_set_games(4);
    sim_run(script, settings, on_shot);
}

int main(int argc, char **argv)
{
    uint16_t cheats_on[SETTINGS_WORDS];

    if (argc < 3) {
        fprintf(stderr, "usage: %s sd.img golden.txt [--update] [--screens DIR]\n", argv[0]);
        return 2;
    }
    s_image = argv[1];
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--update") == 0) {
            s_update = true;
        } else if (strcmp(argv[i], "--screens") == 0 && i + 1 < argc) {
            s_screens_dir = argv[++i];
        }
    }
    if (!host_assets_load(REPO_ROOT)) {
        return 2;
    }
    load_golden(argv[2]);

    check_attribute_filter();
    check_root_listing();

    memset(cheats_on, 0xFF, sizeof(cheats_on));
    cheats_on[SETTINGS_WORD_CHEATS] = 1;

    run(BROWSE, NULL);
    run(SCROLL_CONSISTENCY, NULL);
    {
        /* Moving rows must give exactly what a full redraw gives. */
        const golden_t *a = find_actual("scrolled_down");
        const golden_t *b = find_actual("scrolled_redrawn");
        const golden_t *c = find_actual("scrolled_back");
        EXPECT(a && b && c && a->crc == b->crc && b->crc == c->crc);
    }
    run(ATTRIBUTES, NULL);
    run(GAME_PAGE, NULL);
    EXPECT(sim_boot_requests() == 1);
    run(HOMEBREW, NULL);
    run(CHEATS, cheats_on);
    run(RECENT, NULL);
    run(NOR_LIBRARY, NULL);
    run(SETTINGS, NULL);
    {
        /* Leaving Settings saved the changes. */
        const uint16_t *stored = sim_stored_settings();
        EXPECT(stored != NULL);
        if (stored) {
            EXPECT(stored[SETTINGS_WORD_LANGUAGE] == SETTINGS_LANGUAGE_ENGLISH);
            EXPECT(stored[SETTINGS_WORD_RESET_HOOK] == 1);
            EXPECT(stored[SETTINGS_WORD_SAVE_STATE_HOOK] == 1);
            EXPECT(stored[SETTINGS_WORD_SLEEP_HOOK] == 0);
        }
    }
    run(ABOUT, NULL);

    if (s_update) {
        FILE *f = fopen(argv[2], "w");
        if (!f) {
            perror(argv[2]);
            return 2;
        }
        for (unsigned i = 0; i < s_actual_count; i++) {
            fprintf(f, "%-32s %08x\n", s_actual[i].name, (unsigned)s_actual[i].crc);
        }
        fclose(f);
        printf("sim: wrote %u screens to %s\n", s_actual_count, argv[2]);
    } else {
        for (unsigned i = 0; i < s_actual_count; i++) {
            const golden_t *g = find_golden(s_actual[i].name);
            if (!g || g->crc != s_actual[i].crc) {
                fprintf(stderr, "sim: screen %s %s (see " FAILED_SCREENS_DIR ")\n",
                        s_actual[i].name, g ? "differs from golden.txt" : "is not in golden.txt");
                s_failures++;
            }
        }
        if (s_golden_count != s_actual_count) {
            fprintf(stderr, "sim: golden.txt has %u screens, the scenarios took %u\n",
                    s_golden_count, s_actual_count);
            s_failures++;
        }
    }

    printf("sim: %u screens, %u failures\n", s_actual_count, s_failures);
    return s_failures == 0 ? 0 : 1;
}
