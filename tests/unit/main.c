/**
 * @file main.c
 * @brief Runs all unit test suites. Usage: unit_tests [name-filter]
 */
#include <stdio.h>

#include "check.h"
#include "host.h"

int g_check_failures;
int g_check_tests;
const char *g_check_filter;

void suite_text(void);
void suite_settings(void);
void suite_library(void);
void suite_cheat(void);
void suite_patch(void);
void suite_gfx(void);

int main(int argc, char **argv)
{
    g_check_filter = argc > 1 ? argv[1] : NULL;
    if (!host_assets_load(REPO_ROOT)) {
        return 2;
    }

    suite_text();
    suite_settings();
    suite_library();
    suite_cheat();
    suite_patch();
    suite_gfx();

    printf("%d tests, %d failures\n", g_check_tests, g_check_failures);
    return g_check_failures == 0 ? 0 : 1;
}
