"""Author the standalone W aura without changing shared VFX assets."""
import unreal as u
from pathlib import Path
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
Path(u.Paths.project_content_dir(),'VFX/Gragas').mkdir(parents=True,exist_ok=True)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world,'LOL.Gragas.BuildWAura')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/VFX/Gragas'],True)
assert isinstance(u.load_asset('/Game/VFX/Gragas/P_GragasWAura'),u.ParticleSystem)
print('GRAGAS_W_AURA_READY')
