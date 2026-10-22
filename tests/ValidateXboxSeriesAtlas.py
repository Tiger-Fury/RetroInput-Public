from pathlib import Path
import json
import re
from PIL import Image, ImageStat
root = Path(__file__).resolve().parent.parent
image = Image.open(root/'assets/buttons/xbox-series/atlas.png')
data = json.loads((root/'assets/buttons/xbox-series/import-map.json').read_text())
icons = data['icons']
assert image.size == (1024,1024) and image.mode == 'RGBA'
assert len(icons) == 38
assert len({tuple(e['source_cell']) for e in icons}) == 38
assert {tuple(e['source_cell']) for e in icons} == {
    (col,row) for row in range(11) for col in range(4)
    if row != 1 and (row < 10 or col < 2)
}
header = (root/'RetroInput/Core/ButtonImages.h').read_text()
body = re.search(r'enum class Button\s*\{(.*?)\bCount\b',header,re.S).group(1)
names = {v.strip() for v in body.split(',') if v.strip()}
tiles = {}
for e in icons:
    assert e['button'] in names
    i=e['tile']; x=i%8*128; y=i//8*128
    tile=image.crop((x,y,x+128,y+128))
    assert tile.getbbox() and tile.getchannel('A').getextrema() == (0,255)
    for edge in [(0,0,128,4),(0,124,128,128),(0,0,4,128),(124,0,128,128)]:
        assert not tile.crop(edge).getbbox()
    tiles[e['button']]=tile
for name,target in data['aliases'].items():
    assert name in names and target in tiles
arms={'up':(56,12,72,28),'down':(56,100,72,116),
      'left':(12,56,28,72),'right':(100,56,116,72)}
patterns={'DpadUp':{'up'},'DpadDown':{'down'},'DpadLeft':{'left'},'DpadRight':{'right'},
          'DpadLeftRight':{'left','right'},'DpadUpDown':{'up','down'},'DpadNeutral':set()}
for name,directions in patterns.items():
    for direction,box in arms.items():
        value=ImageStat.Stat(tiles[name].crop(box).convert('RGB')).mean[0]
        assert (value>110)==(direction in directions),(name,direction,value)
print('38 icons, no white faces, API names/aliases, transparency, gutters and seven D-pad patterns passed')
