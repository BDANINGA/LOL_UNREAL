"""Original W energy halo: gold ring, tilted inner orbit and cyan trailing filaments."""
from pathlib import Path
import math
out=Path(__file__).resolve().parent/'Art/Ezreal'
out.mkdir(parents=True,exist_ok=True)
verts,uvs,faces=[],[],[]
def vertex(p,uv):
    verts.append(p);uvs.append(uv);return len(verts)
def quad(a,b,c,d,slot):
    faces.extend([(slot,(a,b,c)),(slot,(a,c,d))])
def ring(rx,ry,tube,tilt,slot):
    rows=[]
    for i in range(73):
        a=i*math.tau/72
        row=[]
        for j in range(9):
            b=j*math.tau/8
            x=math.cos(a)*(rx+tube*math.cos(b))
            y=math.sin(a)*(ry+tube*math.cos(b))
            z=tube*math.sin(b)
            row.append(vertex((x,y*math.cos(tilt)-z*math.sin(tilt),y*math.sin(tilt)+z*math.cos(tilt)),(i/72,j/8)))
        rows.append(row)
    for i in range(72):
        for j in range(8):
            quad(rows[i][j],rows[i+1][j],rows[i+1][j+1],rows[i][j+1],slot)
ring(29,43,2.8,0,'Core')
ring(24,37,1.5,1.05,'Core')
# Broad, soft blue halo follows the main ring, leaving its center open.
for tilt in [0,1.05]:
    rows=[]
    for i in range(73):
        a=i*math.tau/72
        rows.append([vertex((math.cos(a)*(29+s*5), math.sin(a)*(43+s*5)*math.cos(tilt),
                             math.sin(a)*(43+s*5)*math.sin(tilt)),(i/72,v)) for s,v in [(-1,0),(1,1)]])
    for i in range(72): quad(rows[i][0],rows[i+1][0],rows[i+1][1],rows[i][1],'Energy')
# Fine swept-back filaments echo Q's flowing tail rather than a solid sphere.
for side in [-1,1]:
    rows=[]
    for i in range(33):
        t=i/32
        x=-83+102*t
        y=side*(5+33*math.sin(t*math.pi*.85))
        z=5*math.sin(t*math.pi*2+side)
        w=2.4*math.sin(math.pi*t)+.04
        rows.append([vertex((x,y+s*w,z),(t,v)) for s,v in [(-1,0),(1,1)]])
    for i in range(32): quad(rows[i][0],rows[i+1][0],rows[i+1][1],rows[i][1],'Energy')
lines=['# W energy halo, centimeters, forward +X','mtllib ArcaneMissiles.mtl','o SM_EzrealEssenceFlux','s 1']
lines+=['v %.6f %.6f %.6f'%v for v in verts]
lines+=['vt %.6f %.6f'%uv for uv in uvs]
previous=None
for slot,face in faces:
    if slot!=previous:lines.append('usemtl '+slot);previous=slot
    lines.append('f '+' '.join(f'{i}/{i}' for i in face))
(out/'SM_EzrealEssenceFlux.obj').write_text('\n'.join(lines)+'\n',encoding='utf8')
print('W mesh',len(verts),'vertices',len(faces),'triangles')
