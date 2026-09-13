"""PIE checks for lobby widget bindings; run after opening Lobby in PIE."""
import unreal as u
import json

worlds=u.EditorLevelLibrary.get_pie_worlds(False)
assert worlds, 'Start Lobby PIE first'
world=worlds[0]
pc=u.GameplayStatics.get_player_controller(world,0)
assert isinstance(pc,u.PC_Lobby)
widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.load_class(None,'/Game/Lobby/wbp_lobby.wbp_lobby_C'),True)
assert len(widgets)==1
lobby=widgets[0]
state=pc.player_state
nickname=str(state.get_editor_property('nickname'))
assert nickname
report={'nickname':nickname,'start_team':state.get_editor_property('team_id'),'moves':[]}
for target,slotname in [(2,'RightSlot_0'),(1,'LeftSlot_0')]:
    lobby.call_method('UpdateLobbyUI')
    slot=lobby.get_editor_property(slotname)
    join=slot.get_editor_property('BTN_Join')
    assert join.get_editor_property('on_clicked').is_bound()
    join.get_editor_property('on_clicked').broadcast()
    assert state.get_editor_property('team_id')==target
    lobby.call_method('UpdateLobbyUI')
    occupied=lobby.get_editor_property(slotname)
    assert str(occupied.get_editor_property('TXT_Nickname').get_text())==nickname
    report['moves'].append({'team':target,'nickname_visible':str(occupied.get_editor_property('TXT_Nickname').get_visibility())})
assert lobby.get_editor_property('게임시작버튼').get_editor_property('on_clicked').is_bound()
print('LOBBY_UI_TEST_PASSED',json.dumps(report,ensure_ascii=False))
