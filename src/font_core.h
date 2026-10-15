#ifndef AMIGA_UNICODE_FONT_CORE_H
#define AMIGA_UNICODE_FONT_CORE_H

#include <stddef.h>
#include <stdint.h>
#include "utf8.h"
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct {
    uint8_t *pixels; /* grayscale: 0 black, 255 white */
    int width;
    int height;
    int stride;
} GrayCanvas;

typedef struct {
    FT_Library library;
    FT_Face face;
} FontContext;

typedef struct {
    unsigned int codepoints;
    unsigned int rendered;
    unsigned int missing;
} FontStats;

int font_open(FontContext *ctx, const char *ttf_filename);
void font_close(FontContext *ctx);
void font_clear(GrayCanvas *canvas);
int font_draw_text(FontContext *ctx, GrayCanvas *canvas,
                   int x, int baseline, int px_height,
                   const char *text, FontStats *stats);
int font_draw_demo(FontContext *ctx, GrayCanvas *canvas, FontStats *stats);

#endif
