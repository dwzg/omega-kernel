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

#include "core/path.h"
#include "ff.h"
#include "host.h"
#include "loader/buffers.h"
#include "loader/directory.h"
#include "loader/save_files.h"
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

#define LEFT PRESS(BTN_LEFT)
#define RIGHT PRESS(BTN_RIGHT)

/* Root: ATTR CHEAT GBA IMGS SAVER | Advance Castlevania Golden Homebrew
 * Mario Metroid Pokemon The-Legend. */
static const sim_step_t LETTER_JUMP[] = {
    RIGHT,
    RIGHT,
    RIGHT,
    RIGHT,
    RIGHT, /* past the folders */
    WAIT(2),
    SHOT("jump_first_game"),
    RIGHT,
    RIGHT,
    RIGHT,
    RIGHT, /* C G H M */
    DOWN,
    WAIT(2),
    SHOT("jump_m_second"), /* Metroid */
    LEFT,
    WAIT(2),
    SHOT("jump_m_start"), /* back to Mario */
    LEFT,
    WAIT(2),
    SHOT("jump_h"), /* at a group start: Homebrew */
    RIGHT,
    RIGHT,
    RIGHT,
    RIGHT, /* M P T, then nothing further */
    WAIT(2),
    SHOT("jump_last"),
    A,
    WAIT(2),
    B,
    WAIT(2),
    SHOT("jump_last_redrawn"),
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
    DOWN, /* past Favorite */
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
    DOWN, /* past Favorite */
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

/* Start-up opens the folder of the last played game with it selected; B
 * then selects the folder we came from. */
static const sim_step_t RESUME[] = {
    WAIT(2), SHOT("resume_last_game"), B, WAIT(2), SHOT("resume_back_selects_folder"), END,
};

static const sim_step_t RESUME_TOP[] = {
    WAIT(2),
    SHOT("resume_top_folder_game"),
    END,
};

static const sim_step_t RESUME_MISSING[] = {
    WAIT(2),
    SHOT("resume_missing_folder"),
    END,
};

/* File names beyond ASCII: accents, kana, and a name too long for the
 * browser that is opened through its short name. */
static const sim_step_t WORLD_NAMES[] = {
    WAIT(2),
    SHOT("names_resume_kana"), /* the last played game is selected */
    A,
    WAIT(2),
    SHOT("names_kana_game_page"),
    B,
    UP,
    WAIT(2),
    SHOT("names_long_name_selected"), /* shortened in the list */
    A,
    WAIT(2),
    SHOT("names_long_name_game_page"), /* opened by its short name */
    B,
    UP,
    A,
    WAIT(2),
    SHOT("names_accented_game_page"),
    B,
    UP,
    A,
    WAIT(2),
    SHOT("names_accented_folder"),
    END,
};

/* Metroid Fusion has a save and a backup (written before the run). */
static const sim_step_t SAVE_BACKUP[] = {
    WAIT(2),
    A,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    WAIT(2),
    DOWN,
    WAIT(2),
    SHOT("game_page_restore"),
    A,
    WAIT(2),
    SHOT("restore_confirm"),
    A,
    WAIT(2),
    B,
    B,
    END,
};

static const sim_step_t FAVORITES_EMPTY[] = {
    B, DOWN, A, WAIT(2), SHOT("favorites_empty"), END,
};

/* Mark Metroid Fusion, then Advance Wars; the list is sorted by name. */
static const sim_step_t FAVORITES[] = {
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
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    A,
    WAIT(2),
    SHOT("game_page_favorite"),
    B,
    UP,
    UP,
    UP,
    UP,
    UP, /* Advance Wars */
    A,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    A,
    B,
    B,
    DOWN,
    A,
    WAIT(2),
    SHOT("favorites"),
    DOWN,
    A,
    WAIT(2),
    SHOT("favorites_game_page"), /* Metroid Fusion */
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    A,
    B,
    WAIT(2),
    SHOT("favorites_after_remove"),
    END,
};

/* Settings > Box art in list. */
static const sim_step_t BROWSE_ART[] = {
    WAIT(12),
    SHOT("art_folder"), /* ATTR: no art */
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN,
    DOWN, /* Metroid Fusion, scrolled */
    WAIT(12),
    SHOT("art_game"),
    UP,
    WAIT(1),
    SHOT("art_moving"), /* cleared until the selection rests */
    DOWN,
    WAIT(12),
    A,
    WAIT(2),
    B,
    WAIT(12),
    SHOT("art_game_redrawn"),
    END,
};

static const sim_step_t ABOUT[] = {
    B, DOWN, DOWN, DOWN, DOWN, DOWN, A, WAIT(2), SHOT("about"), END,
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
        EXPECT(strcmp(directory_name(0), "No Archive Bit.gba") == 0);
        EXPECT(strcmp(directory_name(1), "Normal.gba") == 0);
        EXPECT(strcmp(directory_name(2), "Read Only.gba") == 0);
    }
}

/**
 * Names beyond ASCII are listed as UTF-8, and a name too long for the
 * browser is replaced by its 8.3 short name, which still opens the file.
 */
static void check_world_names(void)
{
    dir_listing_t listing;
    bool found_accented = false;
    bool found_long = false;

    fresh_card();
    EXPECT(directory_read("/GBA/World", &listing));
    EXPECT(listing.folders == 1 && listing.files == 3);
    for (unsigned i = 0; i < listing.folders + listing.files; i++) {
        const char *name = directory_name(i);
        found_accented |= strcmp(name, "Pok\xC3\xA9mon - Version \xC3\x89meraude.gba") == 0;
        found_long |= strstr(name, "[v1.2].gba") != NULL; /* kept in full */
        EXPECT(strcmp(directory_open_name(i), name) == 0);
    }
    EXPECT(found_accented);
    EXPECT(found_long);
}

/**
 * A name that would make the path longer than 255 bytes is opened by its
 * 8.3 short name; saves still use the full name.
 */
static void check_short_name_fallback(void)
{
    char folder[PATH_MAX_LEN] = "/";
    char path[PATH_MAX_LEN];
    char game[160];
    dir_listing_t listing;
    FIL f;

    fresh_card();
    memset(folder + 1, 'F', 150);
    folder[151] = '\0';
    memset(game, 'G', 120);
    strcpy(game + 120, ".gba");
    EXPECT(f_mkdir(folder) == FR_OK);
    /* The full path is too long for the kernel's buffers: create it from inside. */
    EXPECT(f_chdir(folder) == FR_OK);
    EXPECT(f_open(&f, game, FA_WRITE | FA_CREATE_NEW) == FR_OK);
    f_close(&f);
    EXPECT(f_chdir("/") == FR_OK);

    EXPECT(directory_read(folder, &listing) && listing.files == 1);
    EXPECT(strcmp(directory_name(0), game) == 0);
    EXPECT(strchr(directory_open_name(0), '~') != NULL);
    EXPECT(path_join(path, sizeof(path), folder, directory_open_name(0)));
    EXPECT(f_open(&f, path, FA_READ) == FR_OK);
    f_close(&f);
    char full[FF_LFN_BUF + 1];
    sd_long_name(path, full, sizeof(full));
    EXPECT(strcmp(full, game) == 0);
}

/** Many games in one folder: far more than the old limit of 512. */
static void check_large_folder(void)
{
    char path[64];
    dir_listing_t listing;
    FIL f;

    fresh_card();
    EXPECT(f_mkdir("/Big") == FR_OK);
    for (unsigned i = 0; i < 1500; i++) {
        snprintf(path, sizeof(path), "/Big/Game number %04u.gba", i);
        EXPECT(f_open(&f, path, FA_WRITE | FA_CREATE_NEW) == FR_OK);
        f_close(&f);
    }
    EXPECT(directory_read("/Big", &listing));
    EXPECT(listing.files == 1500 && !listing.truncated);
    EXPECT(strcmp(directory_name(0), "Game number 0000.gba") == 0);
    EXPECT(strcmp(directory_name(1499), "Game number 1499.gba") == 0);
}

/** Write a 1 MiB ROM with @p marker at byte @p at (or none). */
static void write_rom(const char *path, const char *marker, uint32_t at)
{
    static uint8_t rom[1u << 20];
    FIL f;
    UINT written;
    memset(rom, 0, sizeof(rom));
    if (marker) {
        memcpy(rom + at, marker, strlen(marker));
    }
    EXPECT(f_open(&f, path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK);
    EXPECT(f_write(&f, rom, sizeof(rom), &written) == FR_OK && written == sizeof(rom));
    f_close(&f);
}

/** Games missing from the database get their save type from their code, once. */
static void check_save_detection(void)
{
    save_mode_t mode;
    fresh_card();
    /* The marker straddles the first two scan blocks. */
    write_rom("/Hack.gba", "FLASH1M_V103", SCRATCH_SIZE - 8);
    write_rom("/Plain.gba", NULL, 0);

    EXPECT(!save_detected_read("Hack.gba", "ZHAK", 1u << 20, &mode));
    EXPECT(save_mode_for_game(SAVE_CHOICE_AUTO, "Hack.gba", "ZHAK", 1u << 20, NULL, NULL) ==
           SAVE_MODE_DEFAULT); /* no scan without a path */
    EXPECT(save_mode_for_game(SAVE_CHOICE_AUTO, "Hack.gba", "ZHAK", 1u << 20, "/Hack.gba", NULL) ==
           SAVE_MODE_FLASH_128K);
    /* Cached, also for NOR games (no path), and kept when the choice changes. */
    EXPECT(save_mode_for_game(SAVE_CHOICE_AUTO, "Hack.gba", "ZHAK", 1u << 20, NULL, NULL) ==
           SAVE_MODE_FLASH_128K);
    EXPECT(save_choice_write("Hack.gba", SAVE_CHOICE_SRAM));
    EXPECT(save_choice_read("Hack.gba") == SAVE_CHOICE_SRAM);
    EXPECT(save_mode_for_game(SAVE_CHOICE_SRAM, "Hack.gba", "ZHAK", 1u << 20, NULL, NULL) ==
           SAVE_MODE_SRAM);
    EXPECT(save_detected_read("Hack.gba", "ZHAK", 1u << 20, &mode) && mode == SAVE_MODE_FLASH_128K);
    /* A different file under the same name is scanned again. */
    EXPECT(!save_detected_read("Hack.gba", "ZHAK", 2u << 20, &mode));

    EXPECT(save_mode_for_game(SAVE_CHOICE_AUTO, "Plain.gba", "ZPLN", 1u << 20, "/Plain.gba",
                              NULL) == SAVE_MODE_DEFAULT);
    EXPECT(save_detected_read("Plain.gba", "ZPLN", 1u << 20, &mode) && mode == SAVE_MODE_DEFAULT);
    /* Known games never need a scan. */
    EXPECT(save_mode_for_game(SAVE_CHOICE_AUTO, "x.gba", "BPEE", 16u << 20, NULL, NULL) ==
           SAVE_MODE_FLASH_128K);
}

/** Write @p size bytes of @p value to @p path. */
static void write_filled(const char *path, uint8_t value, uint32_t size)
{
    FIL f;
    UINT done;
    memset(g_scratch, value, size);
    EXPECT(f_open(&f, path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK);
    EXPECT(f_write(&f, g_scratch, size, &done) == FR_OK && done == size);
    f_close(&f);
}

/** The first byte of @p path, or -1 if it can't be read. */
static int first_byte(const char *path)
{
    FIL f;
    UINT done = 0;
    uint8_t b = 0;
    if (f_open(&f, path, FA_READ) != FR_OK) {
        return -1;
    }
    f_read(&f, &b, 1, &done);
    f_close(&f);
    return done == 1 ? b : -1;
}

#define FUSION_SAV SD_DIR_SAVES "/Metroid Fusion.sav"
#define FUSION_BAK SD_DIR_SAVES "/Metroid Fusion.bak"

/** Saves are copied before each start; a restore swaps save and backup. */
static void check_save_backup(void)
{
    fresh_card();
    f_mkdir(SD_DIR_SAVES);
    write_filled(FUSION_SAV, 0xFF, 0x2000); /* never written: not backed up */
    EXPECT(!save_file_backup("Metroid Fusion.gba"));
    EXPECT(!save_backup_exists("Metroid Fusion.gba"));

    write_filled(FUSION_SAV, 0x11, 0x20000);
    EXPECT(save_file_backup("Metroid Fusion.gba"));
    EXPECT(save_backup_exists("Metroid Fusion.gba"));
    EXPECT(first_byte(FUSION_BAK) == 0x11);

    EXPECT(save_file_backup("Metroid Fusion.gba")); /* unchanged: kept as is */
    EXPECT(first_byte(FUSION_BAK) == 0x11);

    write_filled(FUSION_SAV, 0x22, 0x20000); /* the game played on */
    EXPECT(save_backup_restore("Metroid Fusion.gba"));
    EXPECT(first_byte(FUSION_SAV) == 0x11 && first_byte(FUSION_BAK) == 0x22);
    EXPECT(save_backup_restore("Metroid Fusion.gba")); /* and back */
    EXPECT(first_byte(FUSION_SAV) == 0x22 && first_byte(FUSION_BAK) == 0x11);

    f_unlink(FUSION_SAV); /* only the backup left */
    EXPECT(save_backup_restore("Metroid Fusion.gba"));
    EXPECT(first_byte(FUSION_SAV) == 0x11 && !save_backup_exists("Metroid Fusion.gba"));
    EXPECT(!save_backup_restore("Metroid Fusion.gba"));
}

/** Prepares the card of the SAVE_BACKUP scenario. */
static void setup_save_backup(void)
{
    f_mkdir(SD_DIR_SAVES);
    write_filled(FUSION_SAV, 0x22, 0x8000);
    write_filled(FUSION_BAK, 0x11, 0x8000);
}

/** Folders sort numbers by value and accented letters with their base letter. */
static void check_natural_order(void)
{
    static const char *const created[] = {
        "Zelda.gba", "Mega Man 10.gba", "\xC3\x89gypte.gba", "Mega Man 2.gba", "egypt.gba",
    };
    static const char *const sorted[] = {
        "egypt.gba", "\xC3\x89gypte.gba", "Mega Man 2.gba", "Mega Man 10.gba", "Zelda.gba",
    };
    char path[PATH_MAX_LEN];
    dir_listing_t listing;
    FIL f;

    fresh_card();
    EXPECT(f_mkdir("/Sort") == FR_OK);
    for (unsigned i = 0; i < 5; i++) {
        EXPECT(path_join(path, sizeof(path), "/Sort", created[i]));
        EXPECT(f_open(&f, path, FA_WRITE | FA_CREATE_NEW) == FR_OK);
        f_close(&f);
    }
    EXPECT(directory_read("/Sort", &listing) && listing.files == 5);
    for (unsigned i = 0; i < listing.files && i < 5; i++) {
        EXPECT(strcmp(directory_name(i), sorted[i]) == 0);
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
    EXPECT(strcmp(directory_name(0), "ATTR") == 0);
    EXPECT(strcmp(directory_name(5), "Advance Wars.gba") == 0);
    EXPECT(directory_size(5) == 4u << 20);
    EXPECT(directory_is_folder(4) && !directory_is_folder(5));
    EXPECT(!directory_read("/missing", &listing));
}

/* -------------------------------------------------------------- main -- */

/** For run_with_history(): keep the card's own recently played list. */
static const char KEEP_HISTORY[] = "";

/**
 * Run @p script on a fresh card whose play history is: none (NULL), the
 * card's own list (KEEP_HISTORY), or just @p last_played.
 */
/** Called on the fresh card of the next run, before it starts. */
static void (*s_setup)(void);

static void run_with_history(const sim_step_t *script, const uint16_t *settings,
                             const char *last_played)
{
    fresh_card();
    sim_nor_set_games(4);
    if (s_setup) {
        s_setup();
        s_setup = NULL;
    }
    if (!last_played) {
        f_unlink(SD_FILE_RECENT);
    } else if (last_played != KEEP_HISTORY) {
        FIL f;
        UINT written;
        f_open(&f, SD_FILE_RECENT, FA_WRITE | FA_CREATE_ALWAYS);
        f_write(&f, last_played, (UINT)strlen(last_played), &written);
        f_write(&f, "\n", 1, &written);
        f_close(&f);
    }
    sim_run(script, settings, on_shot);
}

/** Most scenarios start without a play history, in the top folder. */
static void run(const sim_step_t *script, const uint16_t *settings)
{
    run_with_history(script, settings, NULL);
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
    check_world_names();
    check_short_name_fallback();
    check_large_folder();
    check_save_detection();
    check_save_backup();
    check_natural_order();
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
    s_setup = setup_save_backup;
    run_with_history(SAVE_BACKUP, NULL, "/Metroid Fusion.gba");
    EXPECT(first_byte(FUSION_SAV) == 0x11 && first_byte(FUSION_BAK) == 0x22);
    EXPECT(sim_boot_requests() == 1);
    run(HOMEBREW, NULL);
    run(CHEATS, cheats_on);
    run_with_history(RECENT, NULL, KEEP_HISTORY);
    run_with_history(RESUME, NULL, "/GBA/RPG/Final Fantasy VI Advance.gba");
    run_with_history(RESUME_TOP, NULL, "/Metroid Fusion.gba");
    run_with_history(RESUME_MISSING, NULL, "/GBA/Missing Folder/Game.gba");
    {
        /* A last played game whose folder is gone: start in the top folder. */
        const golden_t *a = find_actual("resume_missing_folder");
        const golden_t *b = find_actual("sd_root");
        EXPECT(a && b && a->crc == b->crc);
    }
    {
        uint16_t art_on[SETTINGS_WORDS];
        memset(art_on, 0xFF, sizeof(art_on));
        art_on[SETTINGS_WORD_LIST_ART] = 1;
        run(BROWSE_ART, art_on);
        /* Scrolling the list column moves rows; that must match a full redraw. */
        const golden_t *a = find_actual("art_game");
        const golden_t *b = find_actual("art_game_redrawn");
        EXPECT(a && b && a->crc == b->crc);
    }
    run(LETTER_JUMP, NULL);
    {
        /* Jumps redraw only what changed; that must match a full redraw. */
        const golden_t *a = find_actual("jump_last");
        const golden_t *b = find_actual("jump_last_redrawn");
        EXPECT(a && b && a->crc == b->crc);
    }
    run(FAVORITES_EMPTY, NULL);
    run(FAVORITES, NULL);
    {
        FIL f;
        char line[64] = "";
        EXPECT(f_open(&f, SD_FILE_FAVORITES, FA_READ) == FR_OK);
        f_gets(line, sizeof(line), &f);
        f_close(&f);
        EXPECT(strcmp(line, "/Advance Wars.gba\n") == 0);
    }
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
    run_with_history(WORLD_NAMES, NULL,
                     "/GBA/World/\xE3\x83\x9D\xE3\x82\xB1\xE3\x83\x83\xE3\x83\x88"
                     "\xE3\x83\xA2\xE3\x83\xB3\xE3\x82\xB9\xE3\x82\xBF\xE3\x83\xBC "
                     "\xE3\x82\xA8\xE3\x83\xA1\xE3\x83\xA9\xE3\x83\xAB\xE3\x83\x89"
                     " (\xE6\x97\xA5\xE6\x9C\xAC).gba"); /* (日本) */
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
