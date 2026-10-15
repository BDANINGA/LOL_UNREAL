"""Create separate Flash/Exhaust assets; shared hit/E materials are left unchanged."""
import unreal as u
from pathlib import Path
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
Path(u.Paths.project_content_dir(), 'VFX/SummonerSpells').mkdir(parents=True, exist_ok=True)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world, 'LOL.Summoner.BuildEffects')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/VFX/SummonerSpells'], True)
for name in ['P_FlashDeparture', 'P_FlashArrival', 'P_ExhaustAura']:
    assert isinstance(u.load_asset('/Game/VFX/SummonerSpells/' + name), u.ParticleSystem), name
print('SUMMONER_EFFECT_ASSETS_READY')
