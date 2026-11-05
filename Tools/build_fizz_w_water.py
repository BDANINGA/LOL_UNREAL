"""Author W weapon and impact systems using the shared Fizz water materials."""
import unreal as u
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world,'LOL.Fizz.BuildWWater')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/VFX/Fizz'],True)
for name in ['P_FizzWWeapon','P_FizzWImpact']:
    assert isinstance(u.load_asset('/Game/VFX/Fizz/'+name),u.ParticleSystem)
print('FIZZ_W_WATER_READY')
