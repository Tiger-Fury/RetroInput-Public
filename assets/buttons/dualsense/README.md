# DualSense artwork

This pack contains 58 supplied icons, excluding white face buttons. It is
selected automatically for PS5 and Backbone. The embedded atlas uses 128px tiles
with transparent gutters; see [shared image editing](../README.md).

Edit `atlas.png` and rebuild to embed changes. `import-map.json` is the tile
catalogue. To regenerate from the original sheet (requires Pillow):

```powershell
python .\tools\ImportDualSenseAtlas.py .\assets\buttons\dualsense\source.png
```
