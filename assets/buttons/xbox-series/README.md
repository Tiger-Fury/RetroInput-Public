# Xbox Series artwork

This pack contains 38 supplied icons, excluding white face buttons. It is
selected automatically for Xbox/XInput and SHIELD. The embedded atlas uses 128px tiles
with transparent gutters; see [shared image editing](../README.md).

Edit `atlas.png` and rebuild to embed changes. `import-map.json` is the tile
catalogue. To regenerate from the original sheet (requires Pillow):

```powershell
python .\tools\ImportXboxSeriesAtlas.py .\assets\buttons\xbox-series\source.png
```

Stick-click requests alias the supplied stick diagrams. Directional stick-click
variants and touchpad have no image in this sheet and return an empty view.
