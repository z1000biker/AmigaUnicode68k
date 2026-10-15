/* Host-side functional test: produces exactly the grayscale buffer that the
 * AmigaOS front end will draw. Only the window/backend differs. */
#include "font_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_secs(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + t.tv_nsec / 1e9;
}

int main(int argc, char **argv)
{
    FontContext ctx;
    GrayCanvas cv;
    FontStats stats;
    FILE *out;
    int i;
    double t0, t1;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s Font.ttf output.pgm\n", argv[0]);
        return 2;
    }
    if (font_open(&ctx, argv[1])) {
        fprintf(stderr, "Cannot open font: %s\n", argv[1]);
        return 2;
    }
    cv.width = 760;
    cv.height = 280;
    cv.stride = 760;
    cv.pixels = malloc((size_t)cv.stride * cv.height);
    if (!cv.pixels) { font_close(&ctx); return 2; }
    if (font_draw_demo(&ctx, &cv, &stats)) {
        fprintf(stderr, "Render failed\n");
        free(cv.pixels); font_close(&ctx); return 2;
    }
    out = fopen(argv[2], "wb");
    if (!out) {
        perror(argv[2]); free(cv.pixels); font_close(&ctx); return 2;
    }
    fprintf(out, "P5\n%d %d\n255\n", cv.width, cv.height);
    for (i = 0; i < cv.height; ++i)
        fwrite(cv.pixels + (size_t)i * cv.stride, 1, cv.width, out);
    fclose(out);
    printf("Initial render: %u Unicode codepoints, %u glyphs, %u missing glyphs\n",
           stats.codepoints, stats.rendered, stats.missing);
    t0 = now_secs();
    for (i = 0; i < 20; ++i) {
        if (font_draw_demo(&ctx, &cv, NULL)) break;
    }
    t1 = now_secs();
    printf("20 repeats: %.3f ms total, %.3f ms per frame (host only)\n",
           (t1 - t0) * 1000.0, (t1 - t0) * 50.0);
    free(cv.pixels);
    font_close(&ctx);
    return (stats.missing != 0) ? 1 : 0;
}
