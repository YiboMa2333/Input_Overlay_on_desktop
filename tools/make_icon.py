# SPDX-License-Identifier: GPL-2.0-only
"""Original keyboard/mouse app icon. Deterministic vector geometry, no assets/fonts."""
from pathlib import Path
import struct
import zlib
ROOT=Path(__file__).resolve().parents[1]


def rounded(x,y,a,b,c,d,r):
    if not (a<=x<=c and b<=y<=d): return False
    cx=max(a+r,min(c-r,x));cy=max(b+r,min(d-r,y))
    return (x-cx)**2+(y-cy)**2<=r*r


def color(x,y):
    if not rounded(x,y,1,1,63,63,12): return (0,0,0,0)
    result=(19,24,33,255)
    for a,b,c,d in [(8,14,23,29),(26,14,41,29),(8,33,23,48),(26,33,41,48)]:
        if rounded(x,y,a,b,c,d,2):
            result=(216,207,69,255) if a==26 and b==14 else (242,240,231,255)
            if a!=26 or b!=14:
                if rounded(x,y,a+2,b+2,c-2,d-2,1): result=(19,24,33,255)
    if rounded(x,y,46,14,57,48,5):
        result=(242,240,231,255)
        if rounded(x,y,48,16,55,46,3): result=(19,24,33,255)
        if 50<=x<=53 and 20<=y<=28: result=(216,207,69,255)
    return result


def png(size):
    samples=4;rows=bytearray()
    for y in range(size):
        rows.append(0)
        for x in range(size):
            total=[0,0,0,0]
            for sy in range(samples):
                for sx in range(samples):
                    rgba=color((x+(sx+.5)/samples)*64/size,(y+(sy+.5)/samples)*64/size)
                    for c in range(4): total[c]+=rgba[c]
            rows.extend(round(total[c]*255/total[3]) if total[3] else 0 for c in range(3))
            rows.append(round(total[3]/(samples*samples)))
    def chunk(kind,data): return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',size,size,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b'')


if __name__=='__main__':
    sizes=(16,20,24,32,48,64,256);images=[png(size) for size in sizes]
    offset=6+len(sizes)*16;directory=bytearray(struct.pack('<HHH',0,1,len(sizes)))
    for size,data in zip(sizes,images):
        directory.extend(struct.pack('<BBBBHHII',size%256,size%256,0,0,1,32,len(data),offset));offset+=len(data)
    (ROOT/'resources').mkdir(exist_ok=True)
    (ROOT/'resources/app.ico').write_bytes(directory+b''.join(images))

    (ROOT/'resources/app-icon.png').write_bytes(images[-1])
