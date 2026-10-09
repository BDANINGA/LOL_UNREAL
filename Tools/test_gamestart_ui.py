"""PIE smoke check: input replacement, preserved click bindings, join panel, empty host guard."""
import unreal as u
worlds=u.EditorLevelLibrary.get_pie_worlds(False); assert worlds
world=worlds[0]; pc=u.GameplayStatics.get_player_controller(world,0)
assert isinstance(pc,u.PC_GameStart)
widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.load_class(None,'/Game/UI/wbp_gamestart.wbp_gamestart_C'),True)
assert len(widgets)==1
widget=widgets[0]
tree=u.find_object(widget,'WidgetTree_0') or u.find_object(widget,'WidgetTree')
assert tree,widget.get_path_name()
def w(name):
    found=u.find_object(tree,name); assert found,name
    return found
nick=w('NicknameInput'); ip=w('IpAddressInput'); overlay=w('Overlay_0')
assert isinstance(nick,u.EditableTextBox) and isinstance(ip,u.EditableTextBox)
assert overlay.get_visibility()==u.SlateVisibility.COLLAPSED
host=w('게임시작버튼'); join=w('게임시작버튼_1')
for b in [host,join]: assert b.get_editor_property('on_clicked').is_bound()
assert ip.get_editor_property('on_text_committed').is_bound()
saved=nick.get_text(); nick.set_text('')
host.get_editor_property('on_clicked').broadcast()
assert 'GameStartMap' in world.get_name()
nick.set_text('UI Preview')
join.get_editor_property('on_clicked').broadcast()
assert overlay.get_visibility()==u.SlateVisibility.VISIBLE
ip.set_text(''); join.get_editor_property('on_clicked').broadcast()
assert 'GameStartMap' in world.get_name()
overlay.set_visibility(u.SlateVisibility.COLLAPSED); nick.set_text(saved)
print('GAMESTART_UI_TEST_PASSED: editable inputs, host/join delegates, join reveal, empty-input guards')
u.SystemLibrary.execute_console_command(world,'shot showui',pc)
