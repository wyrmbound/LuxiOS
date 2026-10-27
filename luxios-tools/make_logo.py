"""Generates the original LuxiOS mascot ("Luxi", an electric lion cub) as SVG.

Writes branding/luxi-icon.svg (square app icon) and branding/luxi-wordmark.svg
(icon + "LuxiOS" text) into the repo given as argv[1].
"""
import math, os, sys

repo = sys.argv[1]
out = os.path.join(repo, 'branding')
os.makedirs(out, exist_ok=True)

GOLD, GOLD_DK, INK, TEAL, NAVY, NAVY2, CREAM = '#F7C948', '#D99A1E', '#1E1B2E', '#2EC4B6', '#1B2045', '#2C3470', '#FFF4D6'

def burst(cx, cy, r_out, r_in, points, rot=0.0, jag=0.0):
    """Zig-zag mane: alternating outer/inner radii, with a kink on each spike for a lightning feel."""
    pts = []
    for i in range(points * 2):
        a = rot + math.pi * i / points
        r = r_out if i % 2 == 0 else r_in
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
        if jag and i % 2 == 0:
            a2 = a + math.pi / points * 0.45
            r2 = r_in + (r_out - r_in) * 0.55
            pts.append((cx + r2 * math.cos(a2) + jag, cy + r2 * math.sin(a2)))
    return 'M' + ' L'.join(f'{x:.1f},{y:.1f}' for x, y in pts) + ' Z'

def mascot(cx=256, cy=276, s=1.0):
    def p(x, y):  # local coords (centered at face) -> canvas
        return f'{cx + x * s:.1f},{cy + y * s:.1f}'
    mane_back = burst(cx, cy - 6 * s, 196 * s, 140 * s, 11, rot=-math.pi / 2, jag=0)
    mane_front = burst(cx, cy - 6 * s, 172 * s, 128 * s, 11, rot=-math.pi / 2 + math.pi / 11, jag=0)
    return f'''
  <path d="{mane_back}" fill="{INK}"/>
  <path d="{mane_front}" fill="#2D2942"/>
  <!-- ears: rounded triangles, teal inner -->
  <path d="M{p(-112,-58)} Q{p(-150,-170)} {p(-60,-130)} Z" fill="{GOLD}" stroke="{INK}" stroke-width="{8*s:.1f}" stroke-linejoin="round"/>
  <path d="M{p(-104,-80)} Q{p(-128,-146)} {p(-74,-122)} Z" fill="{TEAL}"/>
  <path d="M{p(112,-58)} Q{p(150,-170)} {p(60,-130)} Z" fill="{GOLD}" stroke="{INK}" stroke-width="{8*s:.1f}" stroke-linejoin="round"/>
  <path d="M{p(104,-80)} Q{p(128,-146)} {p(74,-122)} Z" fill="{TEAL}"/>
  <!-- head -->
  <ellipse cx="{cx}" cy="{cy}" rx="{122*s:.1f}" ry="{112*s:.1f}" fill="{GOLD}" stroke="{INK}" stroke-width="{8*s:.1f}"/>
  <!-- muzzle -->
  <ellipse cx="{cx}" cy="{cy + 52*s:.1f}" rx="{62*s:.1f}" ry="{40*s:.1f}" fill="{CREAM}"/>
  <!-- forehead bolt (signature mark) -->
  <path d="M{p(8,-100)} L{p(-18,-56)} L{p(2,-56)} L{p(-10,-22)} L{p(22,-70)} L{p(2,-70)} Z" fill="{TEAL}" stroke="{INK}" stroke-width="{4*s:.1f}" stroke-linejoin="round"/>
  <!-- eyes -->
  <ellipse cx="{cx - 50*s:.1f}" cy="{cy + 2*s:.1f}" rx="{24*s:.1f}" ry="{29*s:.1f}" fill="{INK}"/>
  <ellipse cx="{cx + 50*s:.1f}" cy="{cy + 2*s:.1f}" rx="{24*s:.1f}" ry="{29*s:.1f}" fill="{INK}"/>
  <ellipse cx="{cx - 50*s:.1f}" cy="{cy + 8*s:.1f}" rx="{15*s:.1f}" ry="{19*s:.1f}" fill="{TEAL}"/>
  <ellipse cx="{cx + 50*s:.1f}" cy="{cy + 8*s:.1f}" rx="{15*s:.1f}" ry="{19*s:.1f}" fill="{TEAL}"/>
  <circle cx="{cx - 42*s:.1f}" cy="{cy - 8*s:.1f}" r="{8*s:.1f}" fill="#fff"/>
  <circle cx="{cx + 58*s:.1f}" cy="{cy - 8*s:.1f}" r="{8*s:.1f}" fill="#fff"/>
  <!-- cheeks -->
  <ellipse cx="{cx - 88*s:.1f}" cy="{cy + 40*s:.1f}" rx="{16*s:.1f}" ry="{9*s:.1f}" fill="{GOLD_DK}" opacity=".7"/>
  <ellipse cx="{cx + 88*s:.1f}" cy="{cy + 40*s:.1f}" rx="{16*s:.1f}" ry="{9*s:.1f}" fill="{GOLD_DK}" opacity=".7"/>
  <!-- nose + smile -->
  <path d="M{p(-14,30)} Q{p(0,24)} {p(14,30)} Q{p(0,48)} {p(-14,30)} Z" fill="{INK}"/>
  <path d="M{p(0,42)} Q{p(0,60)} {p(-20,62)} M{p(0,42)} Q{p(0,60)} {p(20,62)}" fill="none" stroke="{INK}" stroke-width="{6*s:.1f}" stroke-linecap="round"/>
'''

def bg(w, h, r):
    return f'''
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="{NAVY2}"/><stop offset="1" stop-color="{NAVY}"/>
    </linearGradient>
  </defs>
  <rect width="{w}" height="{h}" rx="{r}" fill="url(#bg)"/>
'''

icon = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512" width="512" height="512">
  <title>Luxi, the LuxiOS mascot</title>{bg(512, 512, 112)}
  <circle cx="256" cy="270" r="226" fill="none" stroke="{GOLD}" stroke-width="6" opacity=".35"/>{mascot()}
</svg>
'''
open(os.path.join(out, 'luxi-icon.svg'), 'w', encoding='utf-8').write(icon)

word = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1200 400" width="1200" height="400">
  <title>LuxiOS</title>{bg(1200, 400, 80)}{mascot(cx=210, cy=212, s=0.72)}
  <text x="420" y="238" font-family="Segoe UI, Helvetica, Arial, sans-serif" font-weight="800" font-size="150" fill="{CREAM}" letter-spacing="-3">Luxi<tspan fill="{GOLD}">OS</tspan></text>
  <text x="426" y="300" font-family="Segoe UI, Helvetica, Arial, sans-serif" font-weight="600" font-size="36" fill="{TEAL}">3DS custom firmware · built on Luma3DS</text>
</svg>
'''
open(os.path.join(out, 'luxi-wordmark.svg'), 'w', encoding='utf-8').write(word)

# Mascot alone on a transparent background (source for the Rosalina sprite)
mascot_only = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="56 70 400 400" width="400" height="400">{mascot()}
</svg>
'''
open(os.path.join(out, 'luxi-mascot.svg'), 'w', encoding='utf-8').write(mascot_only)

# Boot splash art for the 3DS screens (top 400x240, bottom 320x240); make_splash.py converts them
splash_top = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 240" width="400" height="240">{bg(400, 240, 0)}{mascot(cx=110, cy=128, s=0.42)}
  <text x="200" y="128" font-family="Segoe UI, Helvetica, Arial, sans-serif" font-weight="800" font-size="56" fill="{CREAM}" letter-spacing="-1">Luxi<tspan fill="{GOLD}">OS</tspan></text>
  <text x="203" y="156" font-family="Segoe UI, Helvetica, Arial, sans-serif" font-weight="600" font-size="15" fill="{TEAL}">built on Luma3DS</text>
</svg>
'''
open(os.path.join(out, 'splash-top.svg'), 'w', encoding='utf-8').write(splash_top)

bolt = 'M172,70 L132,140 L162,140 L146,190 L196,112 L166,112 Z'
splash_bottom = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 240" width="320" height="240">{bg(320, 240, 0)}
  <path d="{bolt}" transform="translate(-4,-28)" fill="{TEAL}" stroke="{INK}" stroke-width="5" stroke-linejoin="round"/>
  <text x="160" y="206" text-anchor="middle" font-family="Segoe UI, Helvetica, Arial, sans-serif" font-weight="600" font-size="18" fill="{CREAM}">charging up...</text>
</svg>
'''
open(os.path.join(out, 'splash-bottom.svg'), 'w', encoding='utf-8').write(splash_bottom)
print('wrote', out)
