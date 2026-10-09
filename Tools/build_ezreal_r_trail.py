"""Run in the compiled editor after create_ezreal_r_material.py."""
import unreal as u
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world, 'LOL.Ezreal.BuildRTrail')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/Level/ezreal/FX/R'], True)
assert u.load_asset('/Game/Level/ezreal/FX/R/P_EzrealRTrail')
print('EZREAL_R_TRAIL_ASSET_READY')
