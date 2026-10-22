# DualShock 4 artwork

This pack contains 58 supplied icons, excluding white face buttons. It is
selected automatically for PS4 and VX-2. The embedded atlas uses 128px tiles
with transparent gutters; see [shared image editing](../README.md).

Edit `atlas.png` and rebuild to embed changes. `import-map.json` is the tile
catalogue. To regenerate from the original sheet (requires Pillow):

```powershell
python .\tools\ImportDualShock4Atlas.py .\assets\buttons\dualshock4\source.png
```

`Button::View` and `Button::Share` expose the same supplied Share artwork.
