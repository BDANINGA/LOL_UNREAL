"""Run after create_fizz_q_materials.py and an editor build/restart."""
import unreal as u
from pathlib import Path
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
Path(u.Paths.project_content_dir(),'VFX/Fizz').mkdir(parents=True,exist_ok=True)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world,'LOL.Fizz.BuildQWater')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/VFX/Fizz'],True)
assert isinstance(u.load_asset('/Game/VFX/Fizz/P_FizzQWater'),u.ParticleSystem)
print('FIZZ_Q_WATER_READY')
