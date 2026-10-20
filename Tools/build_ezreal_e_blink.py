"""Build only Ezreal E's two one-shot assets, reusing existing procedural materials."""
import unreal as u
from pathlib import Path
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
Path(u.Paths.project_content_dir(), 'Level/ezreal/FX/E').mkdir(parents=True, exist_ok=True)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world, 'LOL.Ezreal.BuildBlink')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/Level/ezreal/FX/E'], True)
for name in ['P_EzrealEDeparture', 'P_EzrealEArrival']:
    asset = u.load_asset('/Game/Level/ezreal/FX/E/' + name)
    assert isinstance(asset, u.ParticleSystem), name
print('EZREAL_E_ASSETS_READY')
