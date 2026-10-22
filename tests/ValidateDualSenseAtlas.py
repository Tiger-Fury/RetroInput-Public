from pathlib import Path
import json
import re
from PIL import Image, ImageStat

root = Path(__file__).resolve().parent.parent
image = Image.open(root / 'assets/buttons/dualsense/atlas.png')
mapping = json.loads((root / 'assets/buttons/dualsense/import-map.json').read_text())
header = (root / 'RetroInput/Core/ButtonImages.h').read_text()
body = re.search(r'enum class Button\s*\{(.*?)\bCount\b', header, re.S).group(1)
names = [name.strip() for name in body.split(',') if name.strip()]
assert names[:len(mapping)] == [entry['button'] for entry in mapping]
assert image.size == (1024, 1024) and image.mode == 'RGBA'
assert len(mapping) == 58
assert len({tuple(entry['source_rect']) for entry in mapping}) == 58
assert {tuple(entry['source_rect'][:2]) for entry in mapping} == {
    (col * 130, row * 130) for row in range(16) for col in range(4)
    if row != 1 and (row < 15 or col < 2)
}
tiles = {}
for index, entry in enumerate(mapping):
    x, y = index % 8 * 128, index // 8 * 128
    tile = image.crop((x, y, x + 128, y + 128))
    assert tile.getbbox() and tile.getchannel('A').getextrema() == (0, 255)
    for edge in [(0,0,128,4),(0,124,128,128),(0,0,4,128),(124,0,128,128)]:
        assert not tile.crop(edge).getbbox()
    tiles[entry['button']] = tile
arms = {'up': (56,16,72,32), 'down': (56,96,72,112),
        'left': (16,56,32,72), 'right': (96,56,112,72)}
expected = {'DpadUp': {'up'}, 'DpadDown': {'down'}, 'DpadLeft': {'left'},
            'DpadRight': {'right'}, 'DpadLeftRight': {'left','right'},
            'DpadUpDown': {'up','down'}}
for name, directions in expected.items():
    for direction, rectangle in arms.items():
        brightness = ImageStat.Stat(tiles[name].crop(rectangle).convert('RGB')).mean[0]
        assert (brightness > 110) == (direction in directions), (name, direction, brightness)
print('All 58 requested icons covered once; white faces excluded; transparency, gutters and six D-pad patterns passed')
