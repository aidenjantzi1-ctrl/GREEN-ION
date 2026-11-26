from pathlib import Path
import argparse
from PIL import Image,ImageDraw
p=argparse.ArgumentParser();p.add_argument("--output",required=True);a=p.parse_args();out=Path(a.output);out.parent.mkdir(parents=True,exist_ok=True)
im=Image.new("RGBA",(256,256),(8,20,32,255));d=ImageDraw.Draw(im);d.rounded_rectangle((14,14,242,242),radius=44,fill=(8,20,32,255),outline=(87,197,143,255),width=8);d.ellipse((62,62,194,194),outline=(123,219,255,255),width=10);d.ellipse((111,111,145,145),fill=(87,197,143,255));d.arc((36,92,220,164),195,345,fill=(87,197,143,255),width=10);d.ellipse((185,117,207,139),fill=(220,236,255,255));im.save(out,format="ICO",sizes=[(16,16),(32,32),(48,48),(64,64),(128,128),(256,256)])
