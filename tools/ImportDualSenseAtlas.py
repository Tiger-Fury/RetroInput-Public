"""Normalize the supplied 4-column sheet into reusable 128px button tiles.

Usage: python tools/ImportDualSenseAtlas.py path/to/supplied-sheet.png
Requires Pillow. Normal Visual Studio builds do not run this importer.
"""
from pathlib import Path
import sys
import json
from PIL import Image

root = Path(__file__).resolve().parent.parent
source = Image.open(sys.argv[1]).convert('RGBA')
if source.size != (488, 2048):
    raise ValueError('This import map expects the supplied 488x2048 sheet')
# Logical order matches Buttons::Button. No game-specific actions live here.
entries = [
    ('South', 2, 0), ('East', 1, 0), ('West', 3, 0), ('North', 0, 0),
    ('LeftShoulder', 0, 2), ('RightShoulder', 1, 2),
    ('LeftTrigger', 2, 2), ('RightTrigger', 3, 2),
    ('DpadUp', 0, 11), ('DpadDown', 1, 11), ('DpadLeft', 2, 11),
    ('DpadRight', 3, 11), ('DpadLeftRight', 2, 12),
    ('Menu', 0, 15), ('View', 1, 15),
    ('LeftStick', 0, 7), ('RightStick', 0, 9), ('DpadUpDown', 1, 12),
]
for side, first_row in [('Left', 3), ('Right', 5)]:
    for col, suffix in enumerate(['', 'AllDirections', 'LeftRight', 'UpDown']):
        entries.append((side + 'StickClick' + suffix, col, first_row))
    for col, suffix in enumerate(['Up', 'Down', 'Left', 'Right']):
        entries.append((side + 'StickClick' + suffix, col, first_row + 1))
for side, first_row in [('Left', 7), ('Right', 9)]:
    for col, suffix in enumerate(['AllDirections', 'LeftRight', 'UpDown'], start=1):
        entries.append((side + 'Stick' + suffix, col, first_row))
    for col, suffix in enumerate(['Up', 'Down', 'Left', 'Right']):
        entries.append((side + 'Stick' + suffix, col, first_row + 1))
entries += [('DpadNeutral', 0, 12), ('Touchpad', 3, 12)]
# Preserve both diagram variants without guessing additional input semantics.
for row, prefix in [(13, 'StickClickDiagram'), (14, 'StickDiagram')]:
    for col, name in enumerate(['Left' + prefix, 'Right' + prefix,
                                'Right' + prefix + 'Alt', 'Left' + prefix + 'Alt']):
        entries.append((name, col, row))
assert len(entries) == 58
assert len({(col, row) for _, col, row in entries}) == 58
assert {(col, row) for _, col, row in entries} == {
    (col, row) for row in range(16) for col in range(4)
    if row != 1 and (row < 15 or col < 2)
}
atlas = Image.new('RGBA', (1024, 1024))
manifest = []
for index, (name, col, row) in enumerate(entries):
    rect = (col * 130, row * 130, col * 130 + 98, row * 130 + 98)
    # Four transparent pixels around every tile prevent bilinear atlas bleeding.
    tile = source.crop(rect).resize((120, 120), Image.Resampling.LANCZOS)
    atlas.paste(tile, (index % 8 * 128 + 4, index // 8 * 128 + 4))
    manifest.append({'button': name, 'source_rect': rect, 'tile': index})
destination = root / 'assets/buttons/dualsense'
destination.mkdir(parents=True, exist_ok=True)
atlas.save(destination / 'atlas.png')
(destination / 'import-map.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('Imported 58 DualSense icons (excluding white faces) into 1024x1024 transparent atlas')
