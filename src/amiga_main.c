/* Native AmigaOS 3.x 68k FreeType/Unicode demo.
 * Runs on the Workbench public screen. Works on AGA and RTG by using
 * graphics.library RastPort calls; intentionally simple first backend.
 * Compile with m68k-amigaos-gcc and a matching 68k libfreetype.a.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/exec.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/view.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/timer.h>

#include "font_core.h"

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Device *TimerBase = NULL;

static struct MsgPort *timer_port = NULL;
static struct timerequest *timer_req = NULL;

static int timer_start(void)
{
    timer_port = CreateMsgPort();
    if (!timer_port) return 0;
    timer_req = (struct timerequest *)CreateIORequest(timer_port, sizeof(*timer_req));
    if (!timer_req) return 0;
    if (OpenDevice(TIMERNAME, UNIT_MICROHZ,
                   (struct IORequest *)timer_req, 0) != 0) return 0;
    TimerBase = timer_req->tr_node.io_Device;
    return 1;
}

static void timer_stop(void)
{
    if (TimerBase) {
        CloseDevice((struct IORequest *)timer_req);
        TimerBase = NULL;
    }
    if (timer_req) {
        DeleteIORequest((struct IORequest *)timer_req);
        timer_req = NULL;
    }
    if (timer_port) {
        DeleteMsgPort(timer_port);
        timer_port = NULL;
    }
}

static uint64_t eclock_ticks(const struct EClockVal *t)
{
    return ((uint64_t)t->ev_hi << 32) | (uint64_t)t->ev_lo;
}

static void benchmark(FontContext *font, GrayCanvas *cv, int repeats)
{
    struct EClockVal start, end;
    ULONG hz, end_hz;
    uint64_t delta;
    unsigned long ms;
    int i;
    FontStats stats;
    if (!TimerBase) {
        puts("timer.device unavailable; cannot benchmark");
        return;
    }
    /* Both reads use the same E-clock; the 64-bit subtraction handles wrap. */
    hz = ReadEClock(&start);
    for (i = 0; i < repeats; ++i) font_draw_demo(font, cv, &stats);
    end_hz = ReadEClock(&end);
    delta = eclock_ticks(&end) - eclock_ticks(&start);
    ms = hz ? (unsigned long)((double)delta * 1000.0 / (double)hz + 0.5) : 0;
    printf("E-clock %lu Hz (end %lu Hz)\n", (unsigned long)hz,
           (unsigned long)end_hz);
    printf("%d complete grayscale renders: %lu ms; %lu ms/render\n",
           repeats, ms, ms / (unsigned long)repeats);
    printf("Glyphs/frame %lu, missing/font %lu\n",
           (unsigned long)stats.rendered, (unsigned long)stats.missing);
}

/* Allocate reference-counted best-match pens from the Workbench palette.
 * On 4-color screens the shades necessarily collapse to fewer levels.
 */
static int acquire_pens(struct Screen *screen, LONG pens[5])
{
    int i;
    for (i = 0; i < 5; ++i) {
        ULONG gray = (ULONG)(i * 255 / 4);
        ULONG rgb32 = gray * 0x01010101UL;
        pens[i] = ObtainBestPenA(screen->ViewPort.ColorMap,
                                 rgb32, rgb32, rgb32, NULL);
        if (pens[i] == -1) return 0;
    }
    return 1;
}

static void release_pens(struct Screen *screen, LONG pens[5])
{
    int i;
    for (i = 0; i < 5; ++i)
        if (pens[i] >= 0)
            ReleasePen(screen->ViewPort.ColorMap, (ULONG)pens[i]);
}

static unsigned char gray_bucket(uint8_t value)
{
    return (unsigned char)(((unsigned)value * 4u + 127u) / 255u);
}

/* Functional AGA/RTG backend. RLE rows avoid one WritePixel() per pixel.
 * This is correct but not fast; future backend: chunky/RTG WritePixelArray.
 */
static void blit_grayscale(struct Window *win, const GrayCanvas *cv,
                           LONG pens[5])
{
    struct RastPort *rp = win->RPort;
    int y, x, left = win->BorderLeft, top = win->BorderTop;
    int w = win->Width - win->BorderLeft - win->BorderRight;
    int h = win->Height - win->BorderTop - win->BorderBottom;
    if (w > cv->width) w = cv->width;
    if (h > cv->height) h = cv->height;

    SetDrMd(rp, JAM1);
    for (y = 0; y < h; ++y) {
        const uint8_t *row = cv->pixels + (size_t)y * cv->stride;
        for (x = 0; x < w;) {
            int from = x;
            unsigned char level = gray_bucket(row[x]);
            ++x;
            while (x < w && gray_bucket(row[x]) == level) ++x;
            SetAPen(rp, (ULONG)pens[level]);
            RectFill(rp, left + from, top + y,
                     left + x - 1, top + y);
        }
    }
}


static void timed_blit(struct Window *win, const GrayCanvas *cv, LONG pens[5])
{
    struct EClockVal a, b;
    ULONG hz;
    uint64_t ticks;
    unsigned long usec;
    if (!TimerBase) {
        blit_grayscale(win, cv, pens);
        return;
    }
    hz = ReadEClock(&a);
    blit_grayscale(win, cv, pens);
    ReadEClock(&b);
    ticks = eclock_ticks(&b) - eclock_ticks(&a);
    usec = hz ? (unsigned long)((double)ticks * 1000000.0 / (double)hz + 0.5) : 0;
    printf("Window blit: %lu us (includes rastport/palette drawing)\n", usec);
}

int main(int argc, char **argv)
{
    const char *fontpath = (argc > 1) ? argv[1] : "PROGDIR:Fonts/DejaVuSans.ttf";
    struct Screen *screen = NULL;
    struct Window *win = NULL;
    GrayCanvas cv;
    FontContext font;
    FontStats stats;
    LONG pens[5] = {-1, -1, -1, -1, -1};
    ULONG fast_before, fast_after;
    int win_w, win_h, done = 0;
    int result = 1;
    int has_font = 0;

    memset(&cv, 0, sizeof(cv));
    memset(&font, 0, sizeof(font));
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39);
    if (!IntuitionBase || !GfxBase) {
        puts("Requires AmigaOS 3.0+ intuition.library and graphics.library V39");
        goto cleanup;
    }
    fast_before = AvailMem(MEMF_FAST);
    if (font_open(&font, fontpath)) {
        printf("TTF open failed: %s\n", fontpath);
        puts("Pass an absolute AmigaOS font path or install PROGDIR:Fonts/DejaVuSans.ttf");
        goto cleanup;
    }
    has_font = 1;
    screen = LockPubScreen(NULL);
    if (!screen) { puts("Cannot lock Workbench public screen"); goto cleanup; }
    win_w = screen->Width - 16;
    win_h = screen->Height - screen->BarHeight - 10;
    if (win_w > 810) win_w = 810;
    if (win_h > 330) win_h = 330;
    if (win_w < 320 || win_h < 165) {
        puts("Workbench screen too small (need at least 320 x 165 for demo)");
        goto cleanup;
    }
    win = OpenWindowTags(NULL,
            WA_PubScreen, (ULONG)screen,
            WA_Left, 4, WA_Top, (ULONG)(screen->BarHeight + 2),
            WA_Width, (ULONG)win_w, WA_Height, (ULONG)win_h,
            WA_Title, (ULONG)"Amiga Unicode 68k - FreeType 2",
            WA_Flags, WFLG_ACTIVATE | WFLG_DRAGBAR | WFLG_DEPTHGADGET |
                      WFLG_CLOSEGADGET | WFLG_SMART_REFRESH,
            WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW |
                      IDCMP_VANILLAKEY,
            TAG_DONE);
    if (!win) { puts("OpenWindowTags failed"); goto cleanup; }

    cv.width = win->Width - win->BorderLeft - win->BorderRight;
    cv.height = win->Height - win->BorderTop - win->BorderBottom;
    cv.stride = cv.width;
    cv.pixels = (uint8_t *)AllocVec((ULONG)((size_t)cv.stride * cv.height),
                                    MEMF_FAST);
    if (!cv.pixels) { puts("Not enough FAST RAM for grayscale framebuffer"); goto cleanup; }
    if (!acquire_pens(screen, pens)) { puts("Cannot acquire display pens"); goto cleanup; }
    if (font_draw_demo(&font, &cv, &stats)) { puts("Rendering failed"); goto cleanup; }
    fast_after = AvailMem(MEMF_FAST);
    printf("CPU: native 68k / AmigaOS 3.x, TTF file: %s\n", fontpath);
    printf("Workbench grayscale buffer: %d x %d (%lu bytes FAST)\n",
           cv.width, cv.height, (unsigned long)((size_t)cv.stride * cv.height));
    printf("UTF-8 characters: %lu, rasterized glyphs: %lu, missing: %lu\n",
           (unsigned long)stats.codepoints, (unsigned long)stats.rendered,
           (unsigned long)stats.missing);
    printf("FAST RAM before: %lu, after: %lu, delta: %lu bytes\n",
           (unsigned long)fast_before, (unsigned long)fast_after,
           (unsigned long)(fast_before - fast_after));
    puts("Press B for 8-render benchmark, R to redraw, Q to quit.");
    if (!timer_start()) { puts("Could not open timer.device (only benchmark disabled)"); timer_stop(); }
    timed_blit(win, &cv, pens);
    while (!done) {
        struct IntuiMessage *msg;
        WaitPort(win->UserPort);
        while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)) != NULL) {
            ULONG cl = msg->Class;
            UWORD key = msg->Code;
            ReplyMsg((struct Message *)msg);
            if (cl == IDCMP_CLOSEWINDOW) done = 1;
            else if (cl == IDCMP_REFRESHWINDOW) {
                BeginRefresh(win);
                blit_grayscale(win, &cv, pens);
                EndRefresh(win, TRUE);
            } else if (cl == IDCMP_VANILLAKEY) {
                if (key == 'q' || key == 'Q') done = 1;
                else if (key == 'r' || key == 'R') {
                    font_draw_demo(&font, &cv, NULL);
                    blit_grayscale(win, &cv, pens);
                } else if (key == 'b' || key == 'B') {
                    benchmark(&font, &cv, 8);
                    timed_blit(win, &cv, pens);
                    printf("FAST RAM free now: %lu bytes\n",
                           (unsigned long)AvailMem(MEMF_FAST));
                }
            }
        }
    }
    result = 0;

cleanup:
    timer_stop();
    if (screen) release_pens(screen, pens); /* also handles partial allocation */
    if (cv.pixels) FreeVec(cv.pixels);
    if (win) CloseWindow(win);
    if (screen) UnlockPubScreen(NULL, screen);
    if (has_font) font_close(&font);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    return result;
}