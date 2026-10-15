"""Author the standalone E dash aura without changing shared VFX assets."""
import unreal as u
from pathlib import Path
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
assert u.EditorAssetLibrary.does_asset_exist('/Game/VFX/Gragas/M_GragasDashWisp'), 'Run create_gragas_e_material.py first'
Path(u.Paths.project_content_dir(),'VFX/Gragas').mkdir(parents=True,exist_ok=True)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world,'LOL.Gragas.BuildEAura')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/VFX/Gragas'],True)
assert isinstance(u.load_asset('/Game/VFX/Gragas/P_GragasEDashAura'),u.ParticleSystem)
print('GRAGAS_E_AURA_READY')
