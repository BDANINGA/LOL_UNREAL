import unreal as u
from contextlib import nullcontext
bp=u.load_asset('/Game/ChampionSelectMap/WBP_ChampSlot')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)
tree=u.find_object(bp,'WidgetTree')
root=u.find_object(tree,'CanvasPanel_0')
with nullcontext():
    bp.modify(); tree.modify(); root.modify()
    for name,x,y,w,h,z in [('player_10_frame',0,20,64,64,1),('player_10_icon',2,22,60,60,2)]:
        obj=u.find_object(tree,name)
        obj.modify()
        slot=obj.slot
        slot.set_anchors(u.Anchors(minimum=u.Vector2D(0,0),maximum=u.Vector2D(0,0)))
        slot.set_alignment(u.Vector2D(0,0))
        slot.set_position(u.Vector2D(x,y)); slot.set_size(u.Vector2D(w,h))
        slot.set_auto_size(False); slot.set_z_order(z)
        obj.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
        if name=='player_10_frame':
            b=u.SlateBrush(); b.draw_as=u.SlateBrushDrawType.IMAGE
            b.tint_color=u.SlateColor(specified_color=u.LinearColor(.188,.102,.021,1))
            obj.set_brush(b)
    spacer=u.find_object(tree,'Pick_RowExtent')
    if not spacer: spacer=u.new_object(u.Spacer,outer=tree,name='Pick_RowExtent')
    if not spacer.get_parent(): root.add_child(spacer)
    spacer.set_size(u.Vector2D(64,108))
    spacer.slot.set_position(u.Vector2D(0,0))
    spacer.slot.set_size(u.Vector2D(64,108))
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    # UE generates missing designer GUIDs on first compile; verify a clean second compile.
    u.BlueprintEditorLibrary.compile_blueprint(bp)
pick=u.load_asset('/Game/ChampionSelectMap/wbp_pick')
u.BlueprintEditorLibrary.compile_blueprint(pick)
print('COMPILE_CHECK_COMPLETE')
print('dirty_maps',u.EditorLoadingAndSavingUtils.get_dirty_map_packages())
