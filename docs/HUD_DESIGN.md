# Compact desktop HUD

The requested reference attachment was not accessible in the conversation or
workspace. This implementation follows the written layout and palette; exact
shape/proportion matching against that screenshot remains pending its availability.

## Vector appearance

The design canvas is 338 by 148 logical units. Q/W/E align with A/S/D; a wider
Shift key sits to their left. Ctrl sits below Shift, and a wide Space spans the
three letter columns. Key gaps are 6 units. A 132-unit-high curved mouse sits
to the right, aligned with the top and bottom key edges. No screen-sized or
group-sized background rectangle is drawn.

- Key corner radius: 3.5 units. Main outline: 1.2 units, `#F2F0E7`.
- Dark outside edging: 3.6 units, `#080A0D` at 85% alpha for light backgrounds.
- Idle fill: `#101317` at 74% alpha; pressed fill: opaque `#D8CF45`.
- Centered Segoe UI Semibold labels: 13 units, off-white when idle and dark when
  pressed. No font files, textures or upstream assets are bundled.
- Left/right mouse top halves read button states 0/1. The separate wheel capsule
  reads middle-button state 2, with opaque dark backing so adjacent yellow regions
  do not bleed into it. Wheel movement shows an up/down yellow chevron until the
  input model's 180 ms pulse expires. X1/X2 are captured but not visualized.

The same state snapshot feeds all shapes; multiple keys and mouse regions can
light together. There is no text-event popup, input history or decorative animation
unrelated to actual state.

## Transparency and DPI

`Renderer` draws using a Direct2D DC render target bound to a top-down 32-bit BGRA
DIB. Its alpha mode is premultiplied. Each frame clears to zero alpha, draws only
the HUD vectors/text, then presents through `UpdateLayeredWindow` with
`ULW_ALPHA` and `AC_SRC_ALPHA`. There is no color-key transparency, screenshot
texture or opaque HWND render-target clear. The GDI bitmap, selected object and
memory DC are released with the target on size changes and destruction.

The target uses 96 DPI drawing coordinates and a physical-pixel transform over
the entire HUD. Corner/aspect-locked resizing keeps uniform proportions; unlocked
edge resizing permits nonuniform X/Y scaling of all labels, curves and spacing. The
process requests Per Monitor V2 awareness before creating windows. Initial size
uses `GetDpiForWindow`; `WM_DPICHANGED` applies the recommended bounds, invalidates
the surface and redraws. Settings dimensions are logical units. DirectWrite uses
grayscale antialiasing for alpha composition, avoiding ClearType color fringes.

The HWND is a layered popup/tool window, always topmost over ordinary windows.
Locked mode adds `WS_EX_TRANSPARENT` and `WS_EX_NOACTIVATE`, returns transparent
hit tests and does not activate on mouse clicks. Actual cross-process click-through
and foreground focus have been tested, rather than inferred from styles.
Edit mode removes those flags and draws a nearly transparent alpha catch surface
so empty areas inside the selected group receive mouse input. Physical-pixel
selection chrome draws after the HUD transform, keeping eight handles usable at
each DPI. See [placement behavior](PLACEMENT.md).

## Design previews (not native screenshots)

![Idle vector design preview](hud-idle.svg)

![W+D, left mouse and scroll-up vector design preview](hud-active.svg)

Generate again with `python tools/preview_hud.py`. This package-free documentation
tool reads key rectangles from the production header and mirrors mouse paths;
the SVGs are not runtime assets and are not evidence that Windows rendering works.
SVG text rasterization can differ from DirectWrite.

The generator ran successfully and parsed both SVGs as XML. A portable Windows
toolchain is now available; the native application has been built, run and tested.
Native screenshots of the editing and locked modes are linked in `PLACEMENT.md`.
Physical mixed-DPI monitor transitions remain a manual check. See `VALIDATION.md`.

Implementation references: [Direct2D alpha formats](https://learn.microsoft.com/en-us/windows/win32/direct2d/supported-pixel-formats-and-alpha-modes),
[layered-window composition](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatelayeredwindow),
[per-monitor DPI changes](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged).
