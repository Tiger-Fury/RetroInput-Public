# Shared button images

| Pack | Icons | Default input |
|---|---|---|
| [DualSense](dualsense/README.md) | 58 | PS5, Backbone |
| [PS4](dualshock4/README.md) | 58 | PS4, VX-2 |
| [Xbox Series](xbox-series/README.md) | 38 | Xbox/XInput, SHIELD |
| Nintendo Switch | 45 | Original Joy-Con pair |

The PS5, PS4 and Xbox packs exclude duplicate white face buttons. The Nintendo
sheet has one face-button set; all 45 supplied icons are included, including
SL/SR, Home, Capture and controller diagrams. Packs include platform D-pad and
stick artwork. `atlas.png` is 1024x1024: eight columns of 128x128 tiles, with four
transparent pixels around each tile. `import-map.json` lists the logical buttons
and source crops. `source.png` retains the supplied sheet.

Edit a pack's `atlas.png`, retaining dimensions, tile positions and transparency,
then rebuild the DLL. Visual Studio embeds it directly; no packing command or
runtime image file is needed. To regenerate from a source sheet, use the pack's
importer (requires Pillow). Display sizes are set separately for each game.

`Core/ButtonImages.h` provides `GetButtonImage(Controller, Button)` and explicit
`IconPack` requests. Image views expose immutable straight-alpha ARGB CPU pixels;
game renderers upload those pixels and draw the returned region.

## Legacy fallback

`controller-buttons.png` remains the 256x128 fallback with 32x32 tiles and supports
the `xbox` / `playstation` overrides. After editing that PNG, run:

```powershell
.\tools\PackButtonImages.ps1
```

Commit the PNG and generated `RetroInput/Core/ButtonImageData.h`, then rebuild. The fallback
is still used when a platform pack cannot load; it is separate from the current
128px packs. See [prompt settings](../../docs/GLYPHS.md) for artwork overrides.


Regenerate the Nintendo pack with `python tools/ImportNintendoAtlas.py source.png`.
Its source sheet is 632x1976; the logical mapping is in `nintendo/import-map.json`.
Artwork remains independent of controller input support.
