"""Restyle lobby assets without replacing gameplay-bound widgets or graphs.

Close designers before editing; UE 5.7 preview worlds must not enter an Undo transaction.
Run with Tools/unreal.ps1 exec -ScriptFile Tools/restyle_lobby.py.
"""
import unreal as u
from pathlib import Path

assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
editors=u.get_editor_subsystem(u.AssetEditorSubsystem)
main=u.load_asset('/Game/Lobby/wbp_lobby')
slotbp=u.load_asset('/Game/Lobby/wbp_lobby_slot')
for asset in (main,slotbp): editors.close_all_editors_for_asset(asset)

def color(s,alpha=1):
    s=s.lstrip('#')
    rgb=[int(s[i:i+2],16)/255 for i in (0,2,4)]
    return u.LinearColor(*[(v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4) for v in rgb],alpha)

def slate(s,alpha=1): return u.SlateColor(specified_color=color(s,alpha))

def brush(fill,edge=None,width=1,texture=None):
    b=u.SlateBrush(); b.tint_color=slate(fill); b.draw_as=u.SlateBrushDrawType.IMAGE
    if texture: b.resource_object=texture
    if edge:
        b.draw_as=u.SlateBrushDrawType.ROUNDED_BOX
        o=b.outline_settings; o.corner_radii=u.Vector4(0,0,0,0)
        o.rounding_type=u.SlateBrushRoundingType.FIXED_RADIUS
        o.color=slate(edge); o.width=width; b.outline_settings=o
    return b

def new(cls,name):
    obj=u.find_object(tree,name)
    if obj is None: obj=u.new_object(cls,outer=tree,name=name)
    obj.modify()
    return obj

def place(obj,x,y,w,h,z=0,parent=None):
    parent=parent or canvas
    if obj.get_parent()!=parent:
        obj.remove_from_parent(); parent.add_child(obj)
    s=obj.slot; s.set_anchors(u.Anchors(minimum=u.Vector2D(0,0),maximum=u.Vector2D(0,0)))
    s.set_alignment(u.Vector2D(0,0)); s.set_position(u.Vector2D(x,y)); s.set_size(u.Vector2D(w,h))
    s.set_auto_size(False); s.set_z_order(z)
    return obj

def rect(name,x,y,w,h,fill,z=0,edge=None):
    obj=new(u.Image,'Lobby_'+name); obj.set_brush(brush(fill,edge))
    obj.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    return place(obj,x,y,w,h,z)

def text_style(obj,size=24,tint='C8BEA5',align=u.TextJustify.LEFT,bold=False):
    f=obj.get_editor_property('font'); f.font_object=u.load_asset('/Engine/EngineFonts/Roboto')
    f.size=size; f.typeface_font_name='Bold' if bold else 'Regular'; obj.set_font(f)
    obj.set_color_and_opacity(slate(tint)); obj.set_editor_property('justification',align)

def label(name,value,x,y,w,h,size=24,tint='C8BEA5',align=u.TextJustify.LEFT,bold=False,z=4):
    obj=new(u.TextBlock,'Lobby_'+name); obj.set_text(value); text_style(obj,size,tint,align,bold)
    obj.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    return place(obj,x,y,w,h,z)

def button_style(obj,primary=False):
    s=obj.get_editor_property('widget_style')
    s.normal=brush('133646' if primary else '0C202C','C8AA6E' if primary else '53615C',2 if primary else 1)
    s.hovered=brush('1C4C59','F0E6D2',2)
    s.pressed=brush('09232E','0AC8B9',2)
    s.disabled=brush('111B23','343C3F',1)
    s.normal_padding=u.Margin(0,0,0,0); s.pressed_padding=u.Margin(0,0,0,0)
    obj.set_style(s)

# Reusable row: keep BTN_Join and TXT_Nickname, including their visibility logic.
tree=u.find_object(slotbp,'WidgetTree'); slotbp.modify(); tree.modify()
rowroot=u.find_object(tree,'SizeBox_56'); rowroot.modify()
rowroot.set_width_override(538); rowroot.set_height_override(98)
oldrow=u.find_object(tree,'HorizontalBox_48'); oldrow.remove_from_parent()
canvas=new(u.CanvasPanel,'Lobby_RowCanvas'); rowroot.set_content(canvas)
oldrow.set_visibility(u.SlateVisibility.COLLAPSED)
place(oldrow,0,0,1,1,-5)
rect('AvatarFrame',16,19,54,54,'112B38',0,'6F6043')
label('AvatarGlyph','◇',20,18,46,52,31,'C8AA6E',u.TextJustify.CENTER)
nickname=u.find_object(tree,'TXT_Nickname')
place(nickname,91,30,413,44,4); text_style(nickname,25,'F0E6D2',bold=True)
nickname.set_text('소환사')
join=u.find_object(tree,'BTN_Join')
place(join,91,25,416,52,5); button_style(join)
join.set_tool_tip_text('이 팀으로 이동합니다')
jointext=u.find_object(tree,'TextBlock_0')
jointext.set_text('+  팀 참가'); text_style(jointext,21,'C8AA6E',u.TextJustify.CENTER)
join.slot.set_z_order(5)
u.BlueprintEditorLibrary.compile_blueprint(slotbp)
u.BlueprintEditorLibrary.compile_blueprint(slotbp)

# Main surface retains the canvas root and all ten existing slot instances.
tree=u.find_object(main,'WidgetTree'); main.modify(); tree.modify()
root=u.find_object(tree,'CanvasPanel_71'); root.modify()
oldchildren=list(root.get_all_children())
canvas=new(u.CanvasPanel,'Lobby_DesignCanvas')
sizebox=new(u.SizeBox,'Lobby_DesignSize'); sizebox.set_width_override(1920); sizebox.set_height_override(1080)
sizebox.set_content(canvas)
scale=new(u.ScaleBox,'Lobby_ResponsiveScale'); scale.set_content(sizebox)
scale.set_stretch(u.Stretch.SCALE_TO_FIT); scale.set_stretch_direction(u.StretchDirection.BOTH)
if scale.get_parent()!=root: root.add_child(scale)
scale.slot.set_anchors(u.Anchors(minimum=u.Vector2D(0,0),maximum=u.Vector2D(1,1)))
scale.slot.set_offsets(u.Margin(0,0,0,0)); scale.slot.set_z_order(1)
for child in oldchildren:
    if child!=scale:
        child.remove_from_parent(); canvas.add_child(child)

bg=u.find_object(tree,'뒷배경'); place(bg,0,0,1920,1080,-10)
bg.set_color_and_opacity(color('283744')); bg.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
for name in ['Image_0','상단박스','HorizontalBox_1','VerticalBox_1']:
    obj=u.find_object(tree,name)
    if obj: obj.set_visibility(u.SlateVisibility.COLLAPSED)

# Client-like masthead, room title and restrained gold separators.
rect('Masthead',0,0,1920,104,'061019')
rect('MastheadLine',0,103,1920,1,'5A4A2D')
label('Wordmark','LEAGUE OF LEGENDS',58,29,420,45,27,'C8AA6E',bold=True)
label('ActiveSection','플레이',563,32,160,42,27,'F0E6D2',u.TextJustify.CENTER,True)
rect('ActiveUnderline',585,99,116,3,'C8AA6E',2)
label('ModeSection','사용자 설정 게임',790,35,560,38,24,'A09B8C')
label('LobbyStatus','게임 대기실',1510,35,350,38,22,'A09B8C',u.TextJustify.RIGHT)

back=u.find_object(tree,'나가기버튼'); place(back,60,144,58,58,6)
# Retain its existing delegate (if any) and use a text arrow instead of a stretched texture.
button_style(back)
backtext=new(u.TextBlock,'Lobby_BackLabel'); backtext.set_text('‹')
text_style(backtext,40,'C8AA6E',u.TextJustify.CENTER); back.set_content(backtext)
back.set_tool_tip_text('이전 화면')
title=u.find_object(tree,'게임제목텍스트'); place(title,140,139,1065,66,5)
current=str(title.get_text()).strip()
if current in ('Game Lobby',''): title.set_text('소환사의 협곡 대기실')
ets=title.get_editor_property('widget_style')
for state in ['background_image_normal','background_image_hovered','background_image_focused','background_image_read_only']:
    setattr(ets,state,brush('07151E','2D3E42' if 'focused' in state else '07151E'))
ets.foreground_color=slate('F0E6D2')
f=ets.text_style.font; f.size=35; f.typeface_font_name='Bold'; ts=ets.text_style; ts.font=f; ets.text_style=ts
title.set_editor_property('widget_style',ets)
label('RoomSubtitle','사용자 설정  /  5 대 5  /  소환사의 협곡',141,214,1080,39,22,'A09B8C')

# Two 5-player rosters, with fixed geometry even when a slot is hidden by the existing graph.
for side,x,accent,heading,korean in [('Left',60,'39B8CA','블루 팀','1팀박스'),('Right',677,'CB8278','레드 팀','2팀박스')]:
    rect(side+'Panel',x,294,580,566,'071720')
    rect(side+'Rule',x,294,580,2,accent,1)
    label(side+'Heading',heading,x+23,315,370,42,27,accent,bold=True)
    label(side+'Capacity','최대 5명',x+389,321,162,32,19,'8B9089',u.TextJustify.RIGHT)
    rect(side+'Divider',x+22,369,536,1,'2E3E43')
    team=u.find_object(tree,korean); place(team,x+24,370,538,490,8)
    for i in range(5):
        y=370+i*98
        rect(side+'Row'+str(i),x+22,y+4,536,89,'0C1C27',1)
        rect(side+'RowLine'+str(i),x+22,y+93,536,1,'23363F',2)
        slot=u.find_object(tree,side+'Slot_'+str(i))
        assert slot is not None
        slot.modify()
        if slot.get_parent()!=team: slot.remove_from_parent(); team.add_child(slot)
        ss=slot.slot; ss.set_padding(u.Margin(0,0,0,0))
        ss.set_size(u.SlateChildSize(value=1,size_rule=u.SlateSizeRule.AUTOMATIC))
        ss.set_horizontal_alignment(u.HorizontalAlignment.H_ALIGN_FILL)
        ss.set_vertical_alignment(u.VerticalAlignment.V_ALIGN_FILL)

# Map details replace a nonfunctional social sidebar; all information reflects this project.
rect('DetailsPanel',1300,144,560,716,'07131C',0,'3D3B30')
mapframe=rect('MapFrame',1322,168,516,252,'111C25',1,'785A28')
mapimage=new(u.Image,'Lobby_MapArtwork')
mapimage.set_brush(brush('AAB9C7',texture=u.load_asset('/Game/UI/champ-select-planning-intro')))
mapimage.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
place(mapimage,1324,170,512,248,2)
label('MapEyebrow','SUMMONER’S RIFT',1325,444,510,31,19,'C8AA6E',bold=True)
label('MapTitle','소환사의 협곡',1324,487,510,60,38,'F0E6D2',bold=True)
rect('MapDivider',1324,562,510,1,'51452E')
for i,(key,value) in enumerate([('게임 유형','사용자 설정'),('팀 구성','5 대 5'),('다음 단계','챔피언 선택')]):
    y=589+i*65
    label('DetailKey'+str(i),key,1326,y,208,35,21,'8B9089')
    label('DetailValue'+str(i),value,1525,y,309,35,23,'C8BEA5',u.TextJustify.RIGHT)
label('MapFooter','두 팀을 구성하고 협곡으로 향하세요.',1325,795,510,34,20,'738A94')

# Existing host-only start button remains the primary action.
rect('ActionBar',60,890,1800,128,'061019',0,'343B36')
label('ActionTitle','팀을 구성하세요',88,908,800,40,27,'F0E6D2',bold=True)
label('ActionHint','빈 자리의 팀 참가 버튼으로 팀을 변경할 수 있습니다.',88,956,1130,33,21,'A09B8C')
start=u.find_object(tree,'게임시작버튼'); place(start,1371,921,441,67,7); button_style(start,True)
starttext=u.find_object(tree,'TextBlock_99'); starttext.set_text('게임 시작')
text_style(starttext,27,'F0E6D2',u.TextJustify.CENTER,True)
label('StartHint','게임 시작은 방장만 할 수 있습니다',1300,1033,560,31,18,'738A94',u.TextJustify.RIGHT)
label('BottomCaption','소환사의 협곡  ·  사용자 설정 게임',60,1033,980,31,18,'738A94')

u.BlueprintEditorLibrary.compile_blueprint(main)
u.BlueprintEditorLibrary.compile_blueprint(main)
for asset in (slotbp,main):
    assert u.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)
    print('SAVED',asset.get_path_name())
editors.open_editor_for_assets([main])
print('LOBBY_RESTYLE_COMPLETE')
