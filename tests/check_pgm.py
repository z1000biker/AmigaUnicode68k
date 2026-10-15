"""Sanity-check the FreeType grayscale reference output without image libraries."""
import pathlib
import sys

if len(sys.argv) != 2:
    raise SystemExit('usage: python3 tests/check_pgm.py preview.pgm')

path = pathlib.Path(sys.argv[1])
data = path.read_bytes()
header = b'P5\n760 280\n255\n'
assert data.startswith(header), 'unexpected PGM format or geometry'
image = data[len(header):]
assert len(image) == 760 * 280, 'truncated or extra raster bytes'
assert min(image) < 80, 'font not drawn (no dark pixels)'
assert max(image) == 255, 'background missing'
assert sum(px < 255 for px in image) > 700, 'too little glyph content'
print('FreeType render sanity-check: passed (760x280 PGM with text)')
