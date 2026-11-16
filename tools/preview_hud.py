# SPDX-License-Identifier: GPL-2.0-only
"""Generate vector design previews, not screenshots of the Windows application.

Key geometry is read from the production layout. Mouse paths mirror renderer.cpp.
No packages required; these files are documentation, not runtime assets.
"""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
LAYOUT = (ROOT / "src/render/hud_layout.hpp").read_text(encoding="utf-8")
KEYS = re.findall(r'\{L"([^"]+)", (\d+), (\d+), (\d+), (\d+),', LAYOUT)
assert len(KEYS) == 9, "Layout changed: update the preview generator"
BODY = "M278 8 C305 8 322 27 322 58 L322 96 C322 124 304 140 278 140 C252 140 234 124 234 96 L234 58 C234 27 251 8 278 8 Z"
LEFT = "M278 8 C251 8 234 27 234 58 L234 66 L278 66 Z"


def preview(active):
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="676" height="296" viewBox="0 0 338 148">',
             '<title>HUD design preview, not an application screenshot</title>',
             '<g stroke-linejoin="round" font-family="Segoe UI, sans-serif" font-size="13" font-weight="600" text-anchor="middle">']
    for label, x, y, w, h in KEYS:
        pressed = active and label in {"W", "D"}
        fill = "#D8CF45" if pressed else "#101317"
        opacity = "1" if pressed else ".74"
        shape = f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="3.5"'
        parts += [f'{shape} fill="{fill}" fill-opacity="{opacity}" stroke="#080A0D" stroke-opacity=".85" stroke-width="3.6"/>',
                  f'{shape} fill="none" stroke="#F2F0E7" stroke-width="1.2"/>',
                  f'<text x="{int(x)+int(w)/2}" y="{int(y)+int(h)/2}" dominant-baseline="central" fill="{"#151713" if pressed else "#F2F0E7"}">{label}</text>']
    parts += [f'<path d="{BODY}" fill="#101317" fill-opacity=".74"/>']
    if active:
        parts += [f'<path d="{LEFT}" fill="#D8CF45"/>']
    for color, thickness in [("#080A0D", "3.6"), ("#F2F0E7", "1.2")]:
        parts += [f'<path d="{BODY}" fill="none" stroke="{color}" stroke-width="{thickness}"/>',
                  f'<path d="M234 66 H322 M278 9 V24 M278 57 V66" fill="none" stroke="{color}" stroke-width="{thickness}"/>']
    parts += ['<rect x="271" y="25" width="14" height="31" rx="6" fill="#101317" stroke="#080A0D" stroke-width="3.6"/>',
              '<rect x="271" y="25" width="14" height="31" rx="6" fill="none" stroke="#F2F0E7" stroke-width="1.2"/>',
              '<path d="M275 34 H281 M275 40 H281 M275 46 H281" stroke="#F2F0E7" fill="none"/>']
    if active:
        parts += ['<path d="M272 17 L278 13 L284 17" stroke="#080A0D" stroke-width="4" fill="none"/>',
                  '<path d="M272 17 L278 13 L284 17" stroke="#D8CF45" stroke-width="2" fill="none"/>']
    parts += ['</g></svg>']
    result = "\n".join(parts)
    ET.fromstring(result)
    return result


if __name__ == "__main__":
    for active in (False, True):
        name = "hud-active.svg" if active else "hud-idle.svg"
        (ROOT / "docs" / name).write_text(preview(active), encoding="utf-8")
        print(f"Generated docs/{name}")
