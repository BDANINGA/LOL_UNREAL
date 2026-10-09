"""Restrained start-screen polish; preserve existing controls and C++ binding names.
Run in the chosen editor using Tools/unreal.ps1 exec -NodeId ... -ScriptFile ...
"""
import unreal as u
from pathlib import Path

assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
bp=u.load_asset('/Game/UI/wbp_gamestart')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)
tree=u.find_object(bp,'WidgetTree'); root=u.find_object(tree,'CanvasPanel_23')
bp.modify(); tree.modify()

def color(s,a=1):
    v=[int(s[i:i+2],16)/255 for i in (0,2,4)]
    return u.LinearColor(*[(x/12.92 if x<=.04045 else ((x+.055)/1.055)**2.4) for x in v],a)
def slate(s,a=1): return u.SlateColor(specified_color=color(s,a))
def brush(fill,edge=None,width=1,a=1):
    b=u.SlateBrush(); b.tint_color=slate(fill,a); b.draw_as=u.SlateBrushDrawType.IMAGE
    if edge:
        b.draw_as=u.SlateBrushDrawType.ROUNDED_BOX
        o=b.outline_settings; o.corner_radii=u.Vector4(0,0,0,0)
        o.rounding_type=u.SlateBrushRoundingType.FIXED_RADIUS
        o.color=slate(edge); o.width=width; b.outline_settings=o
    return b
def obj(name):
    w=u.find_object(tree,name); assert w,name
    w.modify(); return w
def place(w,x,y,width,height,z=2):
    s=w.slot; s.set_anchors(u.Anchors(minimum=u.Vector2D(.5,.5),maximum=u.Vector2D(.5,.5)))
    s.set_alignment(u.Vector2D(.5,.5)); s.set_position(u.Vector2D(x,y))
    s.set_size(u.Vector2D(width,height)); s.set_auto_size(False); s.set_z_order(z)
def new(cls,name):
    w=u.find_object(tree,'Start_'+name)
    if w is None: w=u.new_object(cls,outer=tree,name='Start_'+name); root.add_child(w)
    w.modify(); w.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE); return w
def rect(name,x,y,width,height,fill,edge=None,z=2):
    w=new(u.Image,name); w.set_brush(brush(fill,edge)); place(w,x,y,width,height,z); return w
def text(w,size,tint,bold=False,spacing=0):
    f=w.get_editor_property('font'); f.size=size; f.typeface_font_name='Bold' if bold else 'Regular'
    f.letter_spacing=spacing; w.set_font(f); w.set_color_and_opacity(slate(tint))
    w.set_editor_property('justification',u.TextJustify.CENTER)
    w.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
def label(name,value,x,y,width,height,size,tint,bold=False,spacing=0):
    w=new(u.TextBlock,name); w.set_text(value); text(w,size,tint,bold,spacing); place(w,x,y,width,height,4)

# Original background and logo remain. Stretch only the full-screen background.
bg=obj('Image_0'); bg.set_color_and_opacity(color('B8C6CC'))
bg.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
bg.slot.set_anchors(u.Anchors(minimum=u.Vector2D(0,0),maximum=u.Vector2D(1,1)))
bg.slot.set_offsets(u.Margin(0,0,0,0)); bg.slot.set_z_order(-2)
panel=obj('Image_1'); panel.set_brush(brush('091420','8C7548',2,.94))
panel.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE); place(panel,0,0,1350,782,0)
rect('InnerFrame',0,0,1334,766,'091420','302F28',1).set_color_and_opacity(color('FFFFFF',.35))
logo=obj('Image_17'); logo.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
logo.slot.set_z_order(6)
rect('HeaderRule',0,-242,380,1,'655638')

# Existing text remains a direct Canvas child: PC_GameStart replaces it at runtime.
label('NicknameLabel','SUMMONER NAME',0,-183,640,34,18,'C8AA6E',True,130)
rect('NicknameField',0,-108,640,82,'060E16','655638')
nickname=obj('text_id'); nickname.set_text('Enter Nickname'); text(nickname,30,'F0E6D2')
place(nickname,0,-108,600,56,5)
label('NicknameHint','Choose your name before entering the lobby',0,-33,780,32,17,'9A9B95')

# Keep the existing ornamental play-button texture, correcting its stretched ratio.
for name,labelname,x,title in [('게임시작버튼','TextBlock_2',-442,'CREATE ROOM'),('게임시작버튼_1','TextBlock_3',442,'JOIN ROOM')]:
    b=obj(name); place(b,x,202,326,88,8)
    style=b.get_editor_property('widget_style')
    base=style.normal
    for key,tint in [('normal','D6C8A9'),('hovered','FFFFFF'),('pressed','86B7C4'),('disabled','505B61')]:
        state=base.copy(); state.tint_color=slate(tint); setattr(style,key,state)
    style.normal_padding=u.Margin(8,0,8,0); style.pressed_padding=u.Margin(8,2,8,0)
    b.set_style(style)
    t=obj(labelname); t.set_text(title); text(t,24,'F0E6D2',True,40)
    b.set_tool_tip_text('Create a lobby as host' if x<0 else 'Enter the host IP address to join')
label('ModeCaption',"SUMMONER'S RIFT  /  CUSTOM GAME",0,326,860,34,17,'9C8964',False,90)

# Preserve Overlay parent for IP replacement and existing show/submit flow.
overlay=obj('Overlay_0'); place(overlay,0,-93,700,280,20)
overlay.set_visibility(u.SlateVisibility.HIDDEN)
ip_panel=obj('Image_202'); ip_panel.set_brush(brush('0A1823','C8AA6E',1))
ip_panel.slot.set_horizontal_alignment(u.HorizontalAlignment.H_ALIGN_FILL)
ip_panel.slot.set_vertical_alignment(u.VerticalAlignment.V_ALIGN_FILL)
ip_panel.slot.set_padding(u.Margin(0,0,0,0)); ip_panel.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
header=obj('TextBlock_142'); header.set_text('ENTER IP ADDRESS'); text(header,22,'C8AA6E',True,70)
header.slot.set_horizontal_alignment(u.HorizontalAlignment.H_ALIGN_FILL)
header.slot.set_vertical_alignment(u.VerticalAlignment.V_ALIGN_TOP)
header.slot.set_padding(u.Margin(24,34,24,0))
ip_bg=obj('Image_204'); ip_bg.set_brush(brush('040C12','665638'))
ip_bg.slot.set_horizontal_alignment(u.HorizontalAlignment.H_ALIGN_FILL)
ip_bg.slot.set_vertical_alignment(u.VerticalAlignment.V_ALIGN_FILL)
ip_bg.slot.set_padding(u.Margin(70,108,70,80)); ip_bg.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
ip=obj('text_ip'); ip.set_text('IP Address'); text(ip,25,'F0E6D2')
ip.slot.set_horizontal_alignment(u.HorizontalAlignment.H_ALIGN_FILL)
ip.slot.set_vertical_alignment(u.VerticalAlignment.V_ALIGN_FILL)
ip.slot.set_padding(u.Margin(84,108,84,80))

u.BlueprintEditorLibrary.compile_blueprint(bp)
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
audit=Path(u.Paths.project_dir(),'Saved/DesignBackups/GameStartBefore_20260914')
task=u.AssetExportTask(); task.object=bp; task.exporter=u.ObjectExporterT3D()
task.filename=str(audit/'after.t3d'); task.automated=True; task.prompt=False
u.Exporter.run_asset_export_task(task)
print('Saved GameStart detail polish')
