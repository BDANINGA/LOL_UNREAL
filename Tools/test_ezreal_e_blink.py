"""Checks real E cast, cooldown rejection, two bursts, optional suppression and cleanup."""
import unreal as u
import json
from pathlib import Path
world = u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc = u.GameplayStatics.get_player_controller(world, 0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)[0], 0)
pawn = u.GameplayStatics.get_player_pawn(world, 0)
if pawn:
    pawn.set_actor_hidden_in_game(True)
    pawn.set_actor_enable_collision(False)
champ = u.GameplayStatics.get_all_actors_of_class(world, u.Champion_Ezreal)[0]
templates = [u.load_asset('/Game/Level/ezreal/FX/E/' + n) for n in ['P_EzrealEDeparture', 'P_EzrealEArrival']]
assert all(templates)
def effects():
    return [c for c in world.get_world_settings().get_components_by_class(u.ParticleSystemComponent)
            if u.SystemLibrary.is_valid(c) and c.get_editor_property('template') in templates]
def cast(target):
    champ.call_method('Server_Skill_E', (target,))
original = champ.get_actor_location()
cast(u.Vector(900, 0, 100))
assert champ.get_actor_location().equals(original) and not effects(), 'Unlearned E produced movement/VFX'
champ.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill', ('E',))
u.GameplayStatics.set_global_time_dilation(world, 0.2)
cast(u.Vector(900, 0, 100))
arrival = champ.get_actor_location()
distance = (arrival-original).length()
assert 1 < distance <= 476, ('E did not blink within its range', distance)
assert len(effects()) == 2, 'Must spawn exactly one burst at each end'
for template, location in zip(templates, [original, arrival]):
    effect = next(c for c in effects() if c.get_editor_property('template') == template)
    assert effect.get_world_location().equals(location)
cast(u.Vector(-900, 0, 100))
assert champ.get_actor_location().equals(arrival) and len(effects()) == 2, 'Cooldown cast produced an extra blink'
result = {'actual_e_cast': True, 'range_limited_distance': distance, 'endpoint_positions': True,
          'unlearned_and_cooldown_rejected': True}
out = Path(u.Paths.project_saved_dir(), 'DesignBackups')
start = u.GameplayStatics.get_time_seconds(world)
state = {'stage': 0}
times = [0.06, 0.18, 0.35]
def tick(dt):
    try:
        age = u.GameplayStatics.get_time_seconds(world) - start
        i = state['stage']
        if i < len(times) and age >= times[i]:
            counts = [c.get_num_active_particles() for c in effects()]
            assert len(counts) == 2 and min(counts) > 0
            result['stage_'+str(i)+'_particles'] = counts
            filename = str(out / ('EzrealE_Stage'+str(i)+'.png')).replace('\\', '/')
            u.SystemLibrary.execute_console_command(world, 'HighResShot 1280x720 filename="'+filename+'"', pc)
            state['stage'] += 1
        elif i == len(times) and age >= 1.0:
            assert not effects(), 'One-shot effects did not clean up'
            champ.set_editor_property('bEnableBlinkEffect', False)
            champ.call_method('Multicast_PlayBlinkEffect', (original, arrival))
            assert not effects()
            champ.set_editor_property('bEnableBlinkEffect', True)
            champ.set_actor_hidden_in_game(True)
            champ.call_method('Multicast_PlayBlinkEffect', (original, arrival))
            assert not effects()
            champ.set_actor_hidden_in_game(False)
            result.update(auto_cleanup=True, disabled_and_hidden_suppressed=True, success=True)
            finish()
    except Exception as exc:
        result.update(success=False, error=str(exc))
        finish()
def finish():
    u.GameplayStatics.set_global_time_dilation(world, 1.0)
    (out/'EzrealEBlinkTest.json').write_text(json.dumps(result, indent=2), encoding='utf8')
    u.unregister_slate_post_tick_callback(handle)
    print('EZREAL_E_RESULT', result)
handle = u.register_slate_post_tick_callback(tick)
print('EZREAL_E_TEST_RUNNING')
