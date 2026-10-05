/**
 * @file check.h
 * @brief A minimal unit test framework for the host tests.
 *
 * A test is a function declared with TEST(name). Test files list their
 * tests in a SUITE, and tests/unit/main.c runs every suite:
 *
 *     TEST(adds_numbers) { CHECK_EQ(4, 2 + 2); }
 *     SUITE(math) { RUN(adds_numbers); }
 *
 * A failed CHECK reports the location and both values, marks the test as
 * failed and returns from it.
 */
#ifndef TESTS_CHECK_H
#define TESTS_CHECK_H

#include <stdio.h>
#include <string.h>

/** Number of failed checks so far (defined in main.c). */
extern int g_check_failures;
/** Number of tests run so far (defined in main.c). */
extern int g_check_tests;
/** Optional name filter from the command line (NULL = run all). */
extern const char *g_check_filter;

#define TEST(name) static void name(void)
#define SUITE(name)                                                                                \
    void suite_##name(void);                                                                       \
    void suite_##name(void)

#define RUN(test)                                                                                  \
    do {                                                                                           \
        if (!g_check_filter || strstr(#test, g_check_filter)) {                                    \
            g_check_tests++;                                                                       \
            test();                                                                                \
        }                                                                                          \
    } while (0)

#define CHECK_FAIL_(...)                                                                           \
    do {                                                                                           \
        g_check_failures++;                                                                        \
        fprintf(stderr, "%s:%d: %s: ", __FILE__, __LINE__, __func__);                              \
        fprintf(stderr, __VA_ARGS__);                                                              \
        fputc('\n', stderr);                                                                       \
        return;                                                                                    \
    } while (0)

/** Check that @p cond is true. */
#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond))                                                                               \
            CHECK_FAIL_("CHECK(%s) failed", #cond);                                                \
    } while (0)

/** Check two integers for equality. */
#define CHECK_EQ(expected, actual)                                                                 \
    do {                                                                                           \
        long long e_ = (long long)(expected), a_ = (long long)(actual);                            \
        if (e_ != a_)                                                                              \
            CHECK_FAIL_("%s == %s: expected %lld (0x%llx), got %lld (0x%llx)", #expected, #actual, \
                        e_, (unsigned long long)e_, a_, (unsigned long long)a_);                   \
    } while (0)

/** Check two strings for equality. */
#define CHECK_STR(expected, actual)                                                                \
    do {                                                                                           \
        const char *e_ = (expected), *a_ = (actual);                                               \
        if (strcmp(e_, a_) != 0)                                                                   \
            CHECK_FAIL_("%s: expected \"%s\", got \"%s\"", #actual, e_, a_);                       \
    } while (0)

/** Check two memory blocks for equality. */
#define CHECK_MEM(expected, actual, size)                                                          \
    do {                                                                                           \
        const unsigned char *e_ = (const void *)(expected), *a_ = (const void *)(actual);          \
        for (size_t i_ = 0; i_ < (size_t)(size); i_++) {                                           \
            if (e_[i_] != a_[i_])                                                                  \
                CHECK_FAIL_("%s differs at byte %zu: expected 0x%02x, got 0x%02x", #actual, i_,    \
                            e_[i_], a_[i_]);                                                       \
        }                                                                                          \
    } while (0)

#endif /* TESTS_CHECK_H */
