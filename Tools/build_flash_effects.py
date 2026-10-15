"""Rebuild only the two Flash assets, preserving Exhaust and champion abilities."""
import unreal as u
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world,'LOL.Summoner.BuildFlash')
for name in ['P_FlashDeparture','P_FlashArrival']:
    assert u.load_asset('/Game/VFX/SummonerSpells/'+name)
print('FLASH_REDESIGN_SAVED')
