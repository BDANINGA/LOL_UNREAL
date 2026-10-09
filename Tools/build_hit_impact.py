"""Editor-only asset creation; no gameplay registration."""
import unreal as u
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world, 'LOL.VFX.BuildHitImpact')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/VFX/HitImpact'], True)
assert u.load_asset('/Game/VFX/HitImpact/P_HitImpact')
print('STANDALONE_HIT_IMPACT_CREATED')
