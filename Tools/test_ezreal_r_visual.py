"""Run after prepare_ezreal_r_preview.py. Checks the real multicast and effect lifetime."""
import unreal as u
from pathlib import Path
import json

world = u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc = u.GameplayStatics.get_player_controller(world, 0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)[0], 0)
pawn = u.GameplayStatics.get_player_pawn(world, 0)
if pawn:
    pawn.set_actor_hidden_in_game(True)
champ = u.GameplayStatics.get_all_actors_of_class(world, u.Champion_Ezreal)[0]
champ.set_actor_hidden_in_game(True)
champ.set_actor_enable_collision(False)
template = u.load_asset('/Game/Level/ezreal/FX/R/P_EzrealRTrail')
output = Path(u.Paths.project_dir(), 'Saved/DesignBackups/EzrealRVisualTest.json')

def emitters():
    return [a for a in u.GameplayStatics.get_all_actors_of_class(world, u.Emitter)
            if a.get_component_by_class(u.ParticleSystemComponent).get_editor_property('template') == template]

def missiles():
    return [a for a in u.GameplayStatics.get_all_actors_of_class(world, u.Actor)
            if a.get_component_by_class(u.EzrealRVisualComponent)]

def fire(kind=2):
    champ.call_method('Multicast_SpawnEzrealProjectile',
                     (kind, u.Vector(-550, 0, 150), u.Vector(650, 0, 150), 1.5, 280.0, 0.0, None, True))

fire()
assert len(missiles()) == 1 and len(emitters()) == 1
missile = missiles()[0]
mesh = missile.get_component_by_class(u.StaticMeshComponent)
assert mesh.static_mesh.get_path_name() == '/Engine/BasicShapes/Plane.Plane'
assert missile.get_component_by_class(u.SphereComponent).get_unscaled_sphere_radius() == 280
start = u.GameplayStatics.get_time_seconds(world)
result = {'r_visual_spawned': True, 'collision_radius_unchanged': True}
state = {'phase': 0}

def tick(dt):
    try:
        elapsed = u.GameplayStatics.get_time_seconds(world) - start
        if state['phase'] == 0 and elapsed >= .6:
            count = emitters()[0].get_component_by_class(u.ParticleSystemComponent).get_num_active_particles()
            assert count > 0
            assert missile.get_actor_location().x > -500
            result['active_particles'] = count
            result['projectile_moves'] = True
            u.SystemLibrary.execute_console_command(world, 'HighResShot 1280x720 filename="' +
                str(Path(u.Paths.project_dir(), 'Saved/DesignBackups/EzrealR_Preview.png')).replace('\\', '/') + '"', pc)
            state['phase'] = 1
        elif state['phase'] == 1 and elapsed >= .95:
            missile.destroy_actor()
            assert len(emitters()) == 1
            result['tail_survives_projectile'] = True
            state['phase'] = 2
        elif state['phase'] == 2 and elapsed >= 1.9:
            assert len(emitters()) == 0
            result['early_destroy_cleanup'] = True
            fire(0)
            fire(1)
            assert len(missiles()) == 0 and len(emitters()) == 0
            result['q_w_have_no_r_effect'] = True
            champ.set_editor_property('bEnableRWaveEffect', False)
            fire()
            assert len(missiles()) == 0 and len(emitters()) == 0
            result['legacy_visual_toggle'] = True
            champ.set_editor_property('bEnableRWaveEffect', True)
            fire()
            state['natural_start'] = elapsed
            state['phase'] = 3
        elif state['phase'] == 3 and elapsed >= state['natural_start'] + 2.4:
            assert len(missiles()) == 0 and len(emitters()) == 0
            result['natural_lifetime_cleanup'] = True
            result['success'] = True
            output.write_text(json.dumps(result, indent=2), encoding='utf8')
            u.unregister_slate_post_tick_callback(handle)
            print('EZREAL_R_VISUAL_TEST_PASSED', result)
    except Exception as exc:
        result.update(success=False, error=str(exc))
        output.write_text(json.dumps(result, indent=2), encoding='utf8')
        u.unregister_slate_post_tick_callback(handle)
        u.log_error('EZREAL_R_VISUAL_TEST_FAILED ' + str(exc))

handle = u.register_slate_post_tick_callback(tick)
print('R_VISUAL_TEST_RUNNING')
