import unreal as u
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for command in ['LOL.Fizz.BuildRFin','LOL.Fizz.BuildRWater']:
    u.SystemLibrary.execute_console_command(world,command)
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/VFX/Fizz'],True)
assert isinstance(u.load_asset('/Game/VFX/Fizz/SM_FizzRFin'),u.StaticMesh)
assert isinstance(u.load_asset('/Game/VFX/Fizz/P_FizzRWake'),u.ParticleSystem)
print('FIZZ_R_ASSETS_READY')
