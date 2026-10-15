/* UTF-8 + FreeType 2 font renderer; no AmigaOS-dependent code in this file.
 * Pixels stay in a compact 8-bit grayscale buffer until the platform backend
 * displays them. This is the first correctness baseline, not a layout engine.
 */
#include "font_core.h"
#include <string.h>


int font_open(FontContext *ctx, const char *ttf_filename)
{
    memset(ctx, 0, sizeof(*ctx));
    if (FT_Init_FreeType(&ctx->library)) return -1;
    if (FT_New_Face(ctx->library, ttf_filename, 0, &ctx->face)) {
        FT_Done_FreeType(ctx->library);
        memset(ctx, 0, sizeof(*ctx));
        return -2;
    }
    if (!(ctx->face->face_flags & FT_FACE_FLAG_SCALABLE)) {
        font_close(ctx);
        return -3;
    }
    return 0;
}

void font_close(FontContext *ctx)
{
    if (ctx->face) FT_Done_Face(ctx->face);
    if (ctx->library) FT_Done_FreeType(ctx->library);
    memset(ctx, 0, sizeof(*ctx));
}

void font_clear(GrayCanvas *canvas)
{
    int y;
    for (y = 0; y < canvas->height; ++y)
        memset(canvas->pixels + (size_t)y * canvas->stride, 255,
               (size_t)canvas->width);
}

static void draw_bitmap(GrayCanvas *canvas, FT_Bitmap *bitmap, int x, int y)
{
    int row, col;
    for (row = 0; row < (int)bitmap->rows; ++row) {
        int yy = y + row;
        const unsigned char *src;
        if (yy < 0 || yy >= canvas->height) continue;
        /* FreeType can return a negative pitch; row 0 is always the top row. */
        src = bitmap->buffer + (ptrdiff_t)row * bitmap->pitch;
        for (col = 0; col < (int)bitmap->width; ++col) {
            int xx = x + col;
            unsigned int a;
            uint8_t *dst;
            if (xx < 0 || xx >= canvas->width) continue;
            if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY) {
                a = src[col];
                if (bitmap->num_grays > 1 && bitmap->num_grays != 256)
                    a = (a * 255u) / (bitmap->num_grays - 1);
            } else if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO) {
                a = (src[col >> 3] & (0x80u >> (col & 7))) ? 255u : 0u;
            } else {
                continue;
            }
            if (a == 0) continue;
            dst = canvas->pixels + (size_t)yy * canvas->stride + xx;
            /* Black text over white. Supports overlapping antialiased glyphs. */
            *dst = (uint8_t)(((unsigned int)*dst * (255u - a) + 127u) / 255u);
        }
    }
}

int font_draw_text(FontContext *ctx, GrayCanvas *canvas,
                   int x, int baseline, int px_height,
                   const char *text, FontStats *stats)
{
    const char *p = text;
    FT_UInt prev = 0;
    FT_Pos pen_x = (FT_Pos)x * 64;

    if (FT_Set_Pixel_Sizes(ctx->face, 0, (FT_UInt)px_height)) return -1;

    while (*p) {
        uint32_t cp = font_utf8_next(&p);
        FT_UInt idx = FT_Get_Char_Index(ctx->face, cp);
        FT_GlyphSlot g;
        FT_Vector kerning;

        if (stats) ++stats->codepoints;
        if (!idx) {
            if (stats) ++stats->missing;
            idx = FT_Get_Char_Index(ctx->face, (FT_ULong)'?');
            if (!idx) { prev = 0; continue; }
        }
        if (prev && FT_HAS_KERNING(ctx->face)) {
            if (FT_Get_Kerning(ctx->face, prev, idx,
                               FT_KERNING_DEFAULT, &kerning) == 0)
                pen_x += kerning.x;
        }
        if (FT_Load_Glyph(ctx->face, idx, FT_LOAD_DEFAULT)) {
            prev = 0;
            continue;
        }
        if (FT_Render_Glyph(ctx->face->glyph, FT_RENDER_MODE_NORMAL)) {
            prev = 0;
            continue;
        }
        g = ctx->face->glyph;
        draw_bitmap(canvas, &g->bitmap,
                    (int)((pen_x + 32) >> 6) + g->bitmap_left,
                    baseline - g->bitmap_top);
        pen_x += g->advance.x;
        prev = idx;
        if (stats) ++stats->rendered;
    }
    return (int)((pen_x + 32) >> 6);
}

int font_draw_demo(FontContext *ctx, GrayCanvas *canvas, FontStats *stats)
{
    static const char *text[] = {
        "Ελληνικά Unicode: Καλημέρα κόσμε!",
        "Πολυτονικό: Ἀθῆναι, Ἑλλάς, Ὀδυσσεύς.",
        "Τόνοι: ά έ ή ί ό ύ ώ ϊ ΐ ΰ · Ω",
        "Latin: Amiga 68k · Français · Español",
        "Σύμβολα: € → ∞ ∑ μ Ω  0123456789"
    };
    int i;
    int usable, step, title_px, body_px, start;

    if (canvas->width < 200 || canvas->height < 120) return -1;
    if (stats) memset(stats, 0, sizeof(*stats));
    font_clear(canvas);

    usable = canvas->height - 20;
    step = usable / 5;
    if (step < 22) step = 22;
    title_px = step >= 49 ? 29 : (step >= 35 ? 23 : 18);
    body_px = step >= 49 ? 23 : (step >= 35 ? 19 : 16);
    start = 8 + title_px;
    for (i = 0; i < 5; ++i) {
        if (font_draw_text(ctx, canvas, 10, start + i * step,
                           i == 0 ? title_px : body_px,
                           text[i], stats) < 0)
            return -2;
    }
    return 0;
}
