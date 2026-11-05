"""Run after create_fizz_e_material.py and the current editor build."""
import unreal as u
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world,'LOL.Fizz.BuildEWater')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/VFX/Fizz'],True)
for name in ['P_FizzETakeoff','P_FizzELanding']:
    assert isinstance(u.load_asset('/Game/VFX/Fizz/'+name),u.ParticleSystem)
assert u.load_asset('/Game/VFX/Fizz/M_FizzERipple')
print('FIZZ_E_WATER_READY')
