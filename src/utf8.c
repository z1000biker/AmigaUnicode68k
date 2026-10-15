/* Strict UTF-8 scalar decoder. Independent of FreeType and AmigaOS. */
#include "utf8.h"

uint32_t font_utf8_next(const char **s)
{
    const unsigned char *p = (const unsigned char *)*s;
    uint32_t c;
    unsigned int need, i;
    uint32_t minvalue;

    if (*p == 0) return 0;
    if (*p < 0x80) { *s += 1; return *p; }
    if (*p >= 0xC2 && *p <= 0xDF) {
        c = *p & 0x1F; need = 2; minvalue = 0x80;
    } else if (*p >= 0xE0 && *p <= 0xEF) {
        c = *p & 0x0F; need = 3; minvalue = 0x800;
    } else if (*p >= 0xF0 && *p <= 0xF4) {
        c = *p & 0x07; need = 4; minvalue = 0x10000;
    } else {
        *s += 1;
        return 0xFFFD;
    }
    for (i = 1; i < need; ++i) {
        if (p[i] == 0 || (p[i] & 0xC0) != 0x80) {
            *s += 1;
            return 0xFFFD;
        }
        c = (c << 6) | (p[i] & 0x3F);
    }
    if (c < minvalue || (c >= 0xD800 && c <= 0xDFFF) || c > 0x10FFFF) {
        *s += 1;
        return 0xFFFD;
    }
    *s += need;
    return c;
}
