"""Verify one-shot playback and capture its stages in the isolated preview scene."""
import unreal as u
from pathlib import Path
import json
world = u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc = u.GameplayStatics.get_player_controller(world, 0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)[0], 0)
pawn = u.GameplayStatics.get_player_pawn(world, 0)
if pawn:
    pawn.set_actor_hidden_in_game(True)
u.GameplayStatics.set_global_time_dilation(world, 0.2)
template = u.load_asset('/Game/VFX/HitImpact/P_HitImpact')
assert template
effect = u.GameplayStatics.spawn_emitter_at_location(world, template, u.Vector(0, 0, 100), auto_destroy=True)
assert effect
start = u.GameplayStatics.get_time_seconds(world)
out = Path(u.Paths.project_saved_dir(), 'DesignBackups')
out.mkdir(parents=True, exist_ok=True)
state = {'stage': 0}
result = {}
times = [0.035, 0.085, 0.16, 0.29]

def tick(dt):
    try:
        age = u.GameplayStatics.get_time_seconds(world) - start
        i = state['stage']
        if i < len(times) and age >= times[i]:
            count = effect.get_num_active_particles()
            assert count > 0, 'No particles at stage ' + str(i)
            result['stage_'+str(i)+'_particles'] = count
            filename = str(out / ('HitImpact_Stage'+str(i)+'.png')).replace('\\', '/')
            u.SystemLibrary.execute_console_command(world, 'HighResShot 960x540 filename="'+filename+'"', pc)
            state['stage'] += 1
        elif i == len(times) and age >= .9:
            assert not u.SystemLibrary.is_valid(effect) or effect.get_num_active_particles() == 0, 'Effect loops or particles do not expire'
            result['single_burst_finishes'] = True
            result['success'] = True
            u.GameplayStatics.set_global_time_dilation(world, 1.0)
            (out/'HitImpactTest.json').write_text(json.dumps(result, indent=2), encoding='utf8')
            u.unregister_slate_post_tick_callback(handle)
            print('HIT_IMPACT_TEST_PASSED', result)
    except Exception as exc:
        result.update(success=False, error=str(exc))
        (out/'HitImpactTest.json').write_text(json.dumps(result, indent=2), encoding='utf8')
        u.GameplayStatics.set_global_time_dilation(world, 1.0)
        u.unregister_slate_post_tick_callback(handle)
        u.log_error('HIT_IMPACT_TEST_FAILED '+str(exc))

handle = u.register_slate_post_tick_callback(tick)
print('HIT_IMPACT_PREVIEW_RUNNING')
