/**
 * @file test_gfx.c
 * @brief Tests for fonts and drawing (gfx/), using the real font files.
 */
#include "check.h"
#include "gfx/font.h"
#include "gfx/gfx.h"

static int count_pixels(uint16_t color)
{
    const uint16_t *fb = gfx_framebuffer();
    int n = 0;
    for (int i = 0; i < GFX_WIDTH * GFX_HEIGHT; i++) {
        n += fb[i] == color;
    }
    return n;
}

TEST(font_metrics)
{
    CHECK_EQ(16, font_line_height(&FONT_BODY));
    CHECK_EQ(14, font_ascent(&FONT_BODY));
    CHECK_EQ(12, font_line_height(&FONT_SMALL));
    CHECK_EQ(16, font_line_height(&FONT_TITLE));
}

TEST(font_unknown_characters_use_question_mark)
{
    glyph_t q;
    glyph_t g;
    font_glyph(&FONT_BODY, '?', &q);
    font_glyph(&FONT_BODY, 0xE9, &g); /* non-ASCII */
    CHECK(q.bitmap == g.bitmap);
    CHECK(q.advance > 0);
}

TEST(text_width_adds_advances)
{
    CHECK_EQ(0, gfx_text_width(&FONT_BODY, ""));
    int a = gfx_text_width(&FONT_BODY, "A");
    CHECK(a > 0);
    CHECK_EQ(3 * a, gfx_text_width(&FONT_BODY, "AAA"));
    CHECK(gfx_text_width(&FONT_TITLE, "W") >= gfx_text_width(&FONT_TITLE, "i"));
}

TEST(text_fit_adds_ellipsis)
{
    char out[64];
    const char *text = "The Legend of Zelda - A Link to the Past";
    CHECK_STR("Short", gfx_text_fit(&FONT_BODY, "Short", 200, out, sizeof(out)));
    gfx_text_fit(&FONT_BODY, text, 80, out, sizeof(out));
    CHECK(gfx_text_width(&FONT_BODY, out) <= 80);
    size_t len = strlen(out);
    CHECK(len > 3);
    CHECK_STR("...", out + len - 3);
}

TEST(pixel_font_is_sharp)
{
    /* Every glyph pixel is fully on or off: text has no blurred edges. */
    for (unsigned c = 33; c < 127; c++) {
        glyph_t g;
        font_glyph(&FONT_BODY, (unsigned char)c, &g);
        int row_bytes = (g.width + 1) / 2;
        for (int i = 0; i < row_bytes * g.height; i++) {
            uint8_t hi = g.bitmap[i] >> 4, lo = g.bitmap[i] & 15;
            CHECK((hi == 0 || hi == 15) && (lo == 0 || lo == 15));
        }
    }
}

TEST(text_draws_inside_bounds)
{
    gfx_fill(0, 0, GFX_WIDTH, GFX_HEIGHT, 0x7FFF);
    int w = gfx_text(&FONT_BODY, 10, 10, 0, "Hello", 0, ALIGN_LEFT);
    CHECK_EQ(gfx_text_width(&FONT_BODY, "Hello"), w);
    CHECK(count_pixels(0) > 10);
    const uint16_t *fb = gfx_framebuffer();
    for (int y = 0; y < GFX_HEIGHT; y++) {
        for (int x = 0; x < GFX_WIDTH; x++) {
            if (fb[y * GFX_WIDTH + x] != 0x7FFF) {
                CHECK(x >= 10 && x < 10 + w + 2 && y >= 10 && y < 10 + 16);
            }
        }
    }
}

TEST(drawing_clips_at_screen_edges)
{
    /* Must neither crash nor write outside the frame buffer (ASan checks). */
    gfx_fill(-20, -20, 400, 400, 0x1234);
    CHECK_EQ(GFX_WIDTH * GFX_HEIGHT, count_pixels(0x1234));
    gfx_text(&FONT_TITLE, 230, 150, 0, "Clipped text", 0, ALIGN_LEFT);
    gfx_text(&FONT_TITLE, -5, -5, 0, "Clipped text", 0, ALIGN_LEFT);
    static uint16_t image[64 * 64];
    gfx_blit(200, 120, 64, 64, image, 64);
    gfx_fill_gradient(-10, 150, 300, 30, 0, 0x7FFF);
    gfx_fill_rounded(220, 140, 40, 40, 4, 0);
}

TEST(fill_gradient_runs_top_to_bottom)
{
    gfx_fill_gradient(0, 0, 10, 10, RGB15(0, 0, 0), RGB15(248, 248, 248));
    const uint16_t *fb = gfx_framebuffer();
    CHECK_EQ(RGB15(0, 0, 0), fb[0]);
    CHECK_EQ(fb[0], fb[9]);
    CHECK((fb[9 * GFX_WIDTH] & 31) > (fb[0] & 31));
}

SUITE(gfx)
{
    RUN(font_metrics);
    RUN(font_unknown_characters_use_question_mark);
    RUN(text_width_adds_advances);
    RUN(text_fit_adds_ellipsis);
    RUN(pixel_font_is_sharp);
    RUN(text_draws_inside_bounds);
    RUN(drawing_clips_at_screen_edges);
    RUN(fill_gradient_runs_top_to_bottom);
}
