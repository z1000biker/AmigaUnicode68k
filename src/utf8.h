#ifndef AMIGA_UNICODE_UTF8_H
#define AMIGA_UNICODE_UTF8_H
#include <stdint.h>
/* Returns the next Unicode scalar value or U+FFFD on invalid input. */
uint32_t font_utf8_next(const char **s);
#endif
