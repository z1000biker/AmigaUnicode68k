# AmigaUnicode68k - FreeType/UTF-8 proof of concept (v0.1)

Native AmigaOS 3.x executable, designed for 68040/060 + FPU and >=64 MB Fast RAM.
No Linux, Chromium, Raspberry Pi or other runtime server is involved.

**This repository contains source code, not a verified Amiga executable.**
The shared UTF-8/FreeType renderer is tested with a Linux host build. The
AmigaOS-specific front end requires cross compilation and testing in WinUAE
or on real hardware before its compatibility can be claimed.

## What works in the code

- UTF-8 decoding with invalid-sequence replacement (U+FFFD).
- TrueType/OpenType glyph lookup and grayscale rasterization using FreeType 2.
- Greek monotonic, precomposed polytonic Greek, Latin and selected symbols.
- Fallback to `?` for missing glyphs (missing count printed to CLI).
- FreeType kerning when the font provides it.
- 8-bit grayscale intermediate surface allocated from **Fast RAM** on AmigaOS.
- Native Intuition window on the Workbench public screen; output via
  `graphics.library` `RectFill` run-length rows (works on AGA and RTG in
  principle, with fewer shades on low-color screens).
- Interactive keys: `B` benchmark 8 rasterization passes, `R` redraw,
  `Q` quit. Timing uses `timer.device` `ReadEClock`; Fast RAM measured with
  `AvailMem(MEMF_FAST)`.

## Deliberate limitations of v0.1

- **No HarfBuzz shaping yet**: precomposed Greek is the target. Canonically
  decomposed combining marks (e.g. alpha + combining acute) and complex scripts
  are not guaranteed to position correctly.
- No line wrapping, bidirectional text, font fallback chains, font family
  selection, or CSS. This is a text rasterizer demonstration, not yet a browser.
- No WOFF/WOFF2; no emoji color fonts.
- AGA uses the Workbench screen palette; antialiasing quality depends on
  available pens. This path uses many `RectFill` calls and must be optimized.
- Benchmarks measure full *off-screen glyph rasterization*, not the screen
  upload (the latter is separate). Output times depend on hardware.
- FreeType's internal allocations use the chosen C runtime allocator. The
  framebuffer itself explicitly uses MEMF_FAST. The before/after Fast RAM
  figures are **not** a high-watermark or a virtual-memory measurement.
- MMU-based swap and hardware FPU/video decoding are *not* implemented here.
- A separate file-supplied font is necessary; no fonts are bundled.

## Set up toolchain

Reference upstream project:
https://github.com/AmigaPorts/m68k-amigaos-gcc

The AmigaPorts toolchain Makefile provides a `libfreetype2` target
(currently identified as FreeType 2.12.1 in that Makefile). From its checkout:

```sh
make all -j$(nproc)
make libfreetype2
# If libfreetype.a needs zlib, also ensure:
make zlib
```

The precise dependencies must match your installed cross toolchain and
multilib selection. This Makefile uses `-lfreetype -lz -lm` by default;
override `FREETYPE_LIBS` if your local build needs extra static libraries.

Place the cross compiler in your PATH, then in this project:

```sh
make amiga
```

The output should be an AmigaOS hunk executable named `AmigaUnicode68k`.
`CPU=68040` is the intended target at present; the compiler flags explicitly
select `-m68040 -mhard-float` to restrict this initial build to 040/060 with
appropriate FPU support. A true 68060-tuned build can be added later with
corresponding FreeType multilib objects.

## Run in WinUAE or on real Amiga

- Set up AmigaOS >= 3.0, an 040/060 CPU configuration with FPU, Fast RAM >=64MB,
  and an AGA or Picasso96 Workbench screen.
- Copy the executable and make a `Fonts` folder beside it.
- Supply your legally obtained Unicode-capable TTF, e.g. DejaVu Sans, as
  `Fonts/DejaVuSans.ttf` (the program defaults to
  `PROGDIR:Fonts/DejaVuSans.ttf`).
- Launch **from a Shell**, so the benchmark and memory readings are visible:

```
AmigaUnicode68k "PROGDIR:Fonts/DejaVuSans.ttf"
```

A second computer is necessary only if you choose to *cross-compile* or run
WinUAE for development. Nothing external is needed to run the program on the
Amiga itself.

## Host validation (Linux)

```sh
make host
./font-preview /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf preview.pgm
```

This produces a portable gray image (`P5` PGM). Any common image viewer can
open it. Use this test to isolate Unicode parsing and glyph rasterization
from the AmigaOS-specific graphics backend.

## Next development checkpoints

1. Verify 68k build with the installed FreeType multilib and test 68040/060.
2. Replace RectFill-per-run with a faster P96 path, plus AGA-optimized blitter.
3. Introduce shaping for decomposed/polytonic text and fallback font chains.
4. Add font metrics/layout suitable for a future HTML/CSS rendering engine.

License for **this sample source only**: MIT (see LICENSE). FreeType and any
user-provided fonts retain their own licenses.