"""Converts rendered splash PNGs into LuxiOS/Luma3DS splash files.

Format (see arm9/source/draw.c): raw BGR8, column-major, each column stored
bottom-to-top: byte offset = (x * 240 + (239 - y)) * 3.
Usage: make_splash.py <top.png> <bottom.png> <outdir>
"""
import os, sys
from PIL import Image

def to_fb(png, width, height=240):
    img = Image.open(png).convert('RGB')
    assert img.size == (width, height), (png, img.size)
    px = img.load()
    out = bytearray(width * height * 3)
    for x in range(width):
        for y in range(height):
            r, g, b = px[x, y]
            o = (x * height + (height - 1 - y)) * 3
            out[o:o + 3] = bytes((b, g, r))
    return bytes(out)

top, bottom, outdir = sys.argv[1:4]
os.makedirs(outdir, exist_ok=True)
open(os.path.join(outdir, 'splash.bin'), 'wb').write(to_fb(top, 400))
open(os.path.join(outdir, 'splashbottom.bin'), 'wb').write(to_fb(bottom, 320))

# Round-trip check: decode splash.bin back to a PNG for visual verification
data = open(os.path.join(outdir, 'splash.bin'), 'rb').read()
img = Image.new('RGB', (400, 240))
for x in range(400):
    for y in range(240):
        o = (x * 240 + (239 - y)) * 3
        b, g, r = data[o:o + 3]
        img.putpixel((x, y), (r, g, b))
img.save(os.path.join(outdir, 'splash-roundtrip.png'))
print('ok', len(data), os.path.getsize(os.path.join(outdir, 'splashbottom.bin')))
