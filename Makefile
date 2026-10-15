# Native AmigaOS 3.x 68k build (requires AmigaPorts GCC and FreeType 2).
AMIGA_CC ?= m68k-amigaos-gcc
HOST_CC ?= cc
CPU ?= 68040
FREETYPE_LIBS ?= -lfreetype -lz -lm
AMIGA_CFLAGS ?= -O2 -std=gnu99 -Wall -Wextra -m$(CPU) -mhard-float -mcrt=nix20
HOST_CFLAGS ?= -O2 -std=c99 -Wall -Wextra -D_POSIX_C_SOURCE=200809L

SOURCES = src/font_core.c src/utf8.c
HEADERS = src/font_core.h src/utf8.h

.PHONY: all amiga host selftest clean
all: amiga
amiga: AmigaUnicode68k

AmigaUnicode68k: src/amiga_main.c $(SOURCES) $(HEADERS)
	$(AMIGA_CC) $(AMIGA_CFLAGS) -o $@ src/amiga_main.c $(SOURCES) $(FREETYPE_LIBS) -mcrt=nix20

selftest: utf8-selftest
	./utf8-selftest

utf8-selftest: tests/test_utf8.c src/utf8.c src/utf8.h
	$(HOST_CC) -O2 -std=c99 -Wall -Wextra -o $@ tests/test_utf8.c src/utf8.c

host: font-preview
font-preview: src/host_main.c $(SOURCES) $(HEADERS)
	$(HOST_CC) $(HOST_CFLAGS) $$(pkg-config --cflags freetype2) -o $@ src/host_main.c $(SOURCES) $$(pkg-config --libs freetype2)

clean:
	rm -f AmigaUnicode68k font-preview utf8-selftest *.pgm *.png
