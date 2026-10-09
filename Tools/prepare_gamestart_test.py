import unreal as u
editor=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not editor.is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(u.load_asset('/Game/UI/wbp_gamestart'))
assert editor.load_level('/Game/GameStart/GameStartMap')
editor.editor_request_begin_play()
print('GameStart PIE requested')
