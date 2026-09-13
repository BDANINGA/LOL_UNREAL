"""Restyle the existing pick widget while retaining bound widgets and graphs.

Run in Unreal Editor using Tools/unreal.ps1 exec -ScriptFile Tools/restyle_pick.py.
Layout uses a 1920x1080 design surface inside a ScaleBox for other resolutions.
"""
import unreal as u
from contextlib import nullcontext

bp = u.load_asset('/Game/ChampionSelectMap/wbp_pick')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)
tree = u.find_object(bp, 'WidgetTree')
root = u.find_object(tree, 'CanvasPanel_46')
assert root is not None

def color(hex_value, alpha=1.0):
    s = hex_value.lstrip('#')
    values = [int(s[i:i+2], 16)/255 for i in (0, 2, 4)]
    return u.LinearColor(*[(v/12.92 if v <= .04045 else ((v+.055)/1.055)**2.4) for v in values], alpha)

def slate(hex_value, alpha=1.0):
    return u.SlateColor(specified_color=color(hex_value, alpha))

def widget(cls, name):
    obj = u.find_object(tree, name)
    if obj is None:
        obj = u.new_object(cls, outer=tree, name=name)
    obj.modify()
    return obj

def place(obj, x, y, w, h, z=0, parent=None):
    parent = parent or canvas
    if obj.get_parent() != parent:
        obj.remove_from_parent()
        parent.add_child(obj)
    slot = obj.slot
    slot.set_anchors(u.Anchors(minimum=u.Vector2D(0,0), maximum=u.Vector2D(0,0)))
    slot.set_alignment(u.Vector2D(0,0))
    slot.set_position(u.Vector2D(x,y))
    slot.set_size(u.Vector2D(w,h))
    slot.set_auto_size(False)
    slot.set_z_order(z)
    return obj

def brush(hex_value, texture=None, edge=None, width=1):
    b = u.SlateBrush()
    b.tint_color = slate(hex_value)
    b.draw_as = u.SlateBrushDrawType.IMAGE
    if texture:
        b.resource_object = texture
    if edge:
        b.draw_as = u.SlateBrushDrawType.ROUNDED_BOX
        settings = b.outline_settings
        settings.corner_radii = u.Vector4(0,0,0,0)
        settings.rounding_type = u.SlateBrushRoundingType.FIXED_RADIUS
        settings.color = slate(edge)
        settings.width = width
        b.outline_settings = settings
    return b

def rect(name, x,y,w,h, fill, z=0):
    obj = widget(u.Image, 'Pick_'+name)
    obj.set_brush(brush(fill))
    obj.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    return place(obj,x,y,w,h,z)

def label(name, value, x,y,w,h,size=24, tint='C8BEA5', align=u.TextJustify.LEFT, weight='Regular', z=5):
    obj = widget(u.TextBlock, 'Pick_'+name)
    obj.set_text(value)
    f = obj.get_editor_property('font')
    f.font_object = u.load_asset('/Engine/EngineFonts/Roboto')
    f.typeface_font_name = weight
    f.size = size
    obj.set_font(f)
    obj.set_color_and_opacity(slate(tint))
    obj.set_editor_property('justification', align)
    obj.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    return place(obj,x,y,w,h,z)

with nullcontext():
    bp.modify()
    tree.modify()
    root.modify()
    original_children = list(root.get_all_children())
    scale = widget(u.ScaleBox, 'Pick_ResponsiveScale')
    canvas = widget(u.CanvasPanel, 'Pick_DesignCanvas')
    sizebox = widget(u.SizeBox, 'Pick_DesignSize')
    sizebox.set_width_override(1920)
    sizebox.set_height_override(1080)
    sizebox.set_content(canvas)
    scale.set_content(sizebox)
    scale.set_stretch(u.Stretch.SCALE_TO_FIT)
    scale.set_stretch_direction(u.StretchDirection.BOTH)
    if scale.get_parent() != root:
        root.add_child(scale)
    scale.slot.set_anchors(u.Anchors(minimum=u.Vector2D(0,0), maximum=u.Vector2D(1,1)))
    scale.slot.set_offsets(u.Margin(0,0,0,0))
    scale.slot.set_z_order(1)
    for obj in original_children:
        if obj != scale:
            obj.remove_from_parent()
            canvas.add_child(obj)

    # Existing illustration remains a subtle background, with clear panel contrast.
    background = u.find_object(tree, '뒷배경')
    place(background,0,0,1920,1080,-10)
    background.set_color_and_opacity(color('425267'))
    background.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    u.find_object(tree,'Image_0').set_visibility(u.SlateVisibility.COLLAPSED)
    u.find_object(tree,'HorizontalBox_0').set_visibility(u.SlateVisibility.COLLAPSED)
    rect('Header',0,0,1920,84,'07121C')
    rect('HeaderRule',0,83,1920,1,'51452E',1)
    label('Brand','LEAGUE OF LEGENDS',52,24,370,38,23,'C8AA6E',weight='Bold')
    label('Mode','소환사의 협곡  /  챔피언 선택',1130,27,730,35,21,'A09B8C',u.TextJustify.RIGHT)

    label('Phase','챔피언을 선택하세요',480,111,960,59,40,'F0E6D2',u.TextJustify.CENTER,'Bold')
    label('PhaseHint','함께 승리할 챔피언을 선택한 뒤 준비를 완료하세요',480,174,960,32,21,'A09B8C',u.TextJustify.CENTER)
    rect('TimerRuleLeft',648,238,233,1,'785A28')
    rect('TimerRuleRight',1039,238,233,1,'785A28')
    timer = u.find_object(tree,'text_timer')
    place(timer,893,203,134,76,6)
    f=timer.get_editor_property('font'); f.size=46; timer.set_font(f)
    timer.set_color_and_opacity(slate('C8AA6E'))
    timer.set_editor_property('justification', u.TextJustify.CENTER)

    # Team containers still receive WBP_ChampSlot from the existing update graph.
    for side, x, accent, title in [('Ally',52,'0AC8B9','아군 팀'), ('Enemy',1524,'C86A64','상대 팀')]:
        rect(side+'Panel',x,288,344,654,'08131D')
        rect(side+'TopLine',x,288,344,2,accent)
        label(side+'Heading',title,x+24,309,290,37,25,accent,weight='Bold')
        for i in range(5):
            y=366+i*108
            rect(side+'Row'+str(i),x+16,y,312,100,'0C1D28')
            rect(side+'RowRule'+str(i),x+16,y+99,312,1,'253740')
            label(side+'Index'+str(i),str(i+1).zfill(2),x+34,y+32,43,30,19,'6F716A')
            label(side+'Slot'+str(i),'팀 슬롯 '+str(i+1),x+144,y+20,163,30,22,'C8BEA5')
            label(side+'Info'+str(i),'챔피언 선택',x+144,y+55,163,27,17,'73848C')
        team=u.find_object(tree,'LeftTeam' if side=='Ally' else 'RightTeam')
        place(team,x+76,366,64,540,10)
    label('AllyFooter','아군의 선택을 확인하세요',76,962,330,30,18,'73848C')
    label('EnemyFooter','상대 팀의 선택 현황',1538,962,330,30,18,'73848C',u.TextJustify.RIGHT)

    rect('PoolPanel',440,288,1040,536,'091722')
    label('PoolHeading','챔피언 목록',472,310,600,38,25,'F0E6D2',weight='Bold')
    label('PoolCount','플레이 가능  10',1110,314,335,30,20,'A09B8C',u.TextJustify.RIGHT)
    rect('PoolRule',472,361,976,1,'3C3C32')
    champions=[('Alistar','알리스타','탱커 · 서포터'),('Blitz','블리츠크랭크','탱커 · 서포터'),('Ezreal','이즈리얼','원거리 · 딜러'),('Fizz','피즈','암살자 · 미드'),('Garen','가렌','전사 · 탑'),('Gragas','그라가스','전사 · 정글'),('Jax','잭스','전사 · 탑'),('LeeSin','리 신','전사 · 정글'),('Tryndamere','트린다미어','전사 · 탑'),('Vayne','베인','원거리 · 딜러')]
    for i,(key,title,role) in enumerate(champions):
        x=480+(i%5)*194; y=390+(i//5)*210
        btn=u.find_object(tree,'Button_'+key)
        assert btn is not None, key
        btn.modify()
        texture=btn.get_editor_property('widget_style').normal.resource_object
        assert texture is not None, key
        rect('PortraitFrame'+key,x-2,y-2,156,156,'785A28',3)
        style=btn.get_editor_property('widget_style')
        style.normal=brush('E1E1E1',texture,'785A28',1)
        style.hovered=brush('FFFFFF',texture,'F0E6D2',3)
        style.pressed=brush('B3E4E0',texture,'0AC8B9',3)
        style.disabled=brush('656B70',texture,'3C3C41',1)
        btn.set_style(style)
        btn.set_tool_tip_text(title+' — '+role)
        place(btn,x,y,152,152,5)
        label('Name'+key,title,x-14,y+157,180,30,21,'C8BEA5',u.TextJustify.CENTER,'Bold')
        # Names sit outside the portraits, keeping the actual click target square.

    rect('ActionPanel',440,848,1040,150,'07121C')
    rect('ActionRule',440,848,1040,1,'51452E')
    label('ActionHeading','선택을 마치셨나요?',474,871,500,38,26,'F0E6D2',weight='Bold')
    label('ActionHint','챔피언 초상화 클릭 → 선택 확정',474,920,510,35,21,'A09B8C')
    ready=u.find_object(tree,'Ready')
    place(ready,1110,885,326,72,7)
    rs=ready.get_editor_property('widget_style')
    rs.normal=brush('102A38',edge='C8AA6E',width=2)
    rs.hovered=brush('173E4A',edge='F0E6D2',width=3)
    rs.pressed=brush('0B202B',edge='0AC8B9',width=2)
    rs.disabled=brush('131B20',edge='4A4A43',width=1)
    ready.set_style(rs)
    ready_text=ready.get_child_at(0)
    ready_text.set_text('선택 확정')
    ready_text.set_color_and_opacity(slate('F0E6D2'))
    f=ready_text.get_editor_property('font'); f.size=26; ready_text.set_font(f)
    rect('Bottom',0,1033,1920,47,'050D14')
    rect('BottomRule',0,1033,1920,1,'34372F')
    label('BottomLabel','소환사의 협곡',52,1044,600,29,17,'A09B8C')
    label('BottomHint','팀원 모두 준비를 완료하면 게임이 시작됩니다',990,1044,870,29,17,'73848C',u.TextJustify.RIGHT)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    print('Pick layout restyled; not saved until validation.')

