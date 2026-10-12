"""Build original +X-forward projectile geometry in centimeters, as editable OBJ sources."""
from pathlib import Path
import math

out = Path(__file__).resolve().parent / 'Art/Ezreal'
out.mkdir(parents=True, exist_ok=True)
(out/'ArcaneMissiles.mtl').write_text('newmtl Core\nKd 1 0.8 0.4\n\nnewmtl Energy\nKd 0.1 0.7 1\n',encoding='utf8')

def build(name, attack=False):
    verts, uvs, faces = [], [], []
    def vertex(p, uv):
        verts.append(p); uvs.append(uv)
        return len(verts)
    def quad(a,b,c,d,material):
        faces.extend([(material,(a,b,c)),(material,(a,c,d))])
    # Rounded, tapered energy head, rather than a flat triangle.
    profile = ([(-32,0.1),(-22,2),(-10,4.5),(2,7),(13,5),(24,0.1)] if attack else
               [(-65,0.1),(-50,2),(-30,5),(-6,9),(16,12),(34,8),(48,4),(60,0.1)])
    rings=[]
    for x,r in profile:
        ring=[]
        for j in range(17):
            a=j*math.tau/16
            ring.append(vertex((x, math.cos(a)*r, math.sin(a)*r*.7),
                               ((x-profile[0][0])/(profile[-1][0]-profile[0][0]),j/16)))
        rings.append(ring)
    for i in range(len(rings)-1):
        for j in range(16):
            quad(rings[i][j],rings[i][j+1],rings[i+1][j+1],rings[i+1][j],'Core')
    # Curved energy filaments wrap around the central bolt. UVs fade both edges.
    for k in range(2 if attack else 3):
        rows=[]
        for i in range(33):
            t=i/32; a=k*math.tau/(2 if attack else 3)+t*(2.2 if attack else 3.8)
            x=(-60+78*t) if attack else (-98+143*t)
            radius=(3+7*math.sin(math.pi*t)) if attack else (4+17*math.sin(math.pi*t))
            width=(1.8 if attack else 3.8)*math.sin(math.pi*t)+.04
            cy,cz=math.cos(a)*radius,math.sin(a)*radius*.7
            rows.append([vertex((x,cy+s*math.cos(a)*width,cz+s*math.sin(a)*width), (t,v))
                         for s,v in [(-1,0),(1,1)]])
        for i in range(32):
            quad(rows[i][0],rows[i+1][0],rows[i+1][1],rows[i][1],'Energy')
    # Two crossed soft wakes keep the silhouette visible from the top-down camera.
    for angle in [0,math.pi/2]:
        rows=[]
        for i in range(25):
            t=i/24
            x=(-67+79*t) if attack else (-100+134*t)
            w=(6 if attack else 14)*math.sin(math.pi*t)**1.2+.02
            rows.append([vertex((x,s*w*math.cos(angle),s*w*math.sin(angle)),(t,v))
                         for s,v in [(-1,0),(1,1)]])
        for i in range(24):
            quad(rows[i][0],rows[i+1][0],rows[i+1][1],rows[i][1],'Energy')
    lines=['# Original Ezreal-inspired arcane projectile, centimeters, forward +X','mtllib ArcaneMissiles.mtl','o '+name,'s 1']
    lines += ['v %.6f %.6f %.6f'%v for v in verts]
    lines += ['vt %.6f %.6f'%uv for uv in uvs]
    previous=None
    for material,face in faces:
        if material!=previous: lines.append('usemtl '+material);previous=material
        lines.append('f '+' '.join(f'{i}/{i}' for i in face))
    (out/(name+'.obj')).write_text('\n'.join(lines)+'\n',encoding='utf8')
    print(name,len(verts),'vertices',len(faces),'triangles')

build('SM_EzrealMysticShot')
build('SM_EzrealBasicBolt',True)
