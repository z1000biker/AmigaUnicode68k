/* Pure-C Unicode decoder tests; no FreeType or Amiga SDK needed. */
#include "../src/utf8.h"
#include <assert.h>
#include <stdio.h>

static void check(const char *input, const uint32_t *expected, unsigned len)
{
    const char *p = input;
    unsigned i;
    for (i = 0; i < len; ++i)
        assert(font_utf8_next(&p) == expected[i]);
    assert(font_utf8_next(&p) == 0);
}

int main(void)
{
    const uint32_t greek[] = {0x0393, 0x03B5, 0x03B9, 0x03AC};
    const uint32_t poly[] = {0x1F08, 0x03B8, 0x1FC6, 0x03BD, 0x03B1, 0x03B9};
    const uint32_t emoji[] = {0x1F642};
    const uint32_t overlong[] = {0xFFFD, 0xFFFD};
    const uint32_t broken[] = {0xFFFD, 0x28, 0xFFFD, 0x28};
    const uint32_t surrogate[] = {0xFFFD, 0xFFFD, 0xFFFD};
    check("Γειά", greek, 4);
    check("Ἀθῆναι", poly, 6);
    check("🙂", emoji, 1);
    check("\xC0\xAF", overlong, 2);
    check("\xF0\x28\x8C\x28", broken, 4);
    check("\xED\xA0\x80", surrogate, 3);
    puts("UTF-8 decoder selftest: passed (Greek, polytonic, 4-byte, invalid)");
    return 0;
}
