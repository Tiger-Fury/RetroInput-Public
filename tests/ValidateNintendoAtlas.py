from pathlib import Path
import json,re
from PIL import Image,ImageChops
r=Path(__file__).resolve().parent.parent;root=r/'assets/buttons/nintendo'
a=Image.open(root/'atlas.png').convert('RGBA');source=Image.open(root/'source.png').convert('RGBA');data=json.loads((root/'import-map.json').read_text())
assert a.size==(1024,1024) and source.size==(632,1976)
assert len(data['icons'])==45 and len({tuple(x['source_cell']) for x in data['icons']})==45
assert {tuple(x['source_cell']) for x in data['icons']}=={(c,y) for y in range(12) for c in range(4) if y<11 or c==0}
for e in data['icons']:
 x=e['tile']%8*128;y=e['tile']//8*128;tile=a.crop((x,y,x+128,y+128))
 assert tile.getbbox()
 expected=Image.new('RGBA',(128,128));expected.paste(source.crop(e['source_rect']).resize((120,120),Image.Resampling.LANCZOS),(4,4))
 assert ImageChops.difference(tile,expected).getbbox() is None,e['button']
 for box in [(0,0,128,4),(0,124,128,128),(0,0,4,128),(124,0,128,128)]:assert tile.crop(box).getchannel('A').getbbox() is None
face={e['button']:e['source_cell'] for e in data['icons'][:4]}
assert face=={'East':[0,0],'South':[1,0],'West':[2,0],'North':[3,0]}
print('All 45 Nintendo icons, complete cell coverage, positional faces, source crops and transparent gutters passed')
