"""PIE integration: real casts, intermediate motion/rotation, arrival, and both detonation paths."""
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
casters = [u.GameplayStatics.get_all_actors_with_tag(world, name)[0] for name in ['RollAuto','RollEarly','RollClose']]
out = Path(u.Paths.project_saved_dir(), 'DesignBackups'); out.mkdir(parents=True, exist_ok=True)
result = {'samples': []}
state = {'phase': 0, 'shots': 0}
u.GameplayStatics.set_global_time_dilation(world, .5)
barrels = []
starts = []
for index, caster in enumerate(casters):
    caster.set_actor_hidden_in_game(False)
    caster.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill', ('Q',))
    start = caster.get_actor_location()
    starts.append(start)
    caster.call_method('Server_Skill_Q', (start + u.Vector(650 if index < 2 else 80, 0, 0),))
    barrel = next((b for b in u.GameplayStatics.get_all_actors_of_class(world, u.GragasQBarrelVisual) if b.get_owner() == caster), None)
    assert barrel and isinstance(barrel, u.GragasQBarrelVisual), 'Real Q did not spawn rolling actor'
    assert abs(barrel.get_actor_location().x-start.x) < .1, 'Spawn position lost'
    barrels.append(barrel)
state['start'] = u.GameplayStatics.get_time_seconds(world)
state['last_x'] = starts[0].x
def rotation_values(barrel):
    rotation = barrel.get_component_by_class(u.StaticMeshComponent).get_editor_property('relative_rotation')
    return tuple(getattr(rotation, axis) for axis in ['pitch','yaw','roll'])

state['initial_rotation'] = rotation_values(barrels[0])

def finish(error=None):
    if error: result['error'] = str(error)
    else: result['success'] = True
    u.GameplayStatics.set_global_time_dilation(world, 1)
    (out/'GragasQRollTest.json').write_text(json.dumps(result, indent=2), encoding='utf8')
    u.unregister_slate_post_tick_callback(handle)
    print('GRAGAS_Q_ROLL_RESULT', result)

def tick(dt):
    try:
        age = u.GameplayStatics.get_time_seconds(world) - state['start']
        if u.SystemLibrary.is_valid(barrels[0]) and age < .95:
            p = barrels[0].get_actor_location()
            assert p.x >= state['last_x']-.01, 'Barrel moved backwards'
            assert 35 < p.z < 55, 'Barrel is floating or below ground'
            state['last_x'] = p.x
        if state['shots'] < 3 and age >= [.18,.5,.85][state['shots']]:
            mesh = barrels[0].get_component_by_class(u.StaticMeshComponent)
            p = barrels[0].get_actor_location()
            rotation = mesh.get_editor_property('relative_rotation')
            assert starts[0].x < p.x < starts[0].x+650
            values = rotation_values(barrels[0])
            assert max(abs(a-b) for a,b in zip(values, state['initial_rotation'])) > 1, ('Mesh is not rolling',values,state['initial_rotation'])
            result['samples'].append({'age':age, 'x':p.x, 'z':p.z, 'rotation':str(rotation)})
            name = out/('GragasQ_Roll_'+str(state['shots'])+'.png')
            u.SystemLibrary.execute_console_command(world, 'HighResShot 1280x720 filename="'+name.as_posix()+'"', pc)
            state['shots'] += 1
        if state['phase'] == 0 and age >= .25:
            casters[1].call_method('Server_Skill_Q', (u.Vector(),))
            assert u.SystemLibrary.is_valid(barrels[1]), 'Early recast exploded ahead of barrel'
            assert abs(barrels[2].get_actor_location().x-(starts[2].x+80)) < .1
            casters[2].call_method('Server_Skill_Q', (u.Vector(),))
            assert not u.SystemLibrary.is_valid(barrels[2]), 'Arrived recast failed'
            result['short_range_and_arrived_recast'] = True
            state['phase'] = 1
        elif state['phase'] == 1 and age >= .8:
            assert u.SystemLibrary.is_valid(barrels[1]), 'Queued detonation was premature'
            state['phase'] = 2
        elif state['phase'] == 2 and age >= 1.15:
            assert not u.SystemLibrary.is_valid(barrels[1]), 'Queued recast failed to detonate on arrival'
            assert u.SystemLibrary.is_valid(barrels[0])
            assert abs(barrels[0].get_actor_location().x-(starts[0].x+650)) < .1
            state['arrival'] = barrels[0].get_actor_location()
            state['arrival_rotation'] = rotation_values(barrels[0])
            result['queued_recast_and_arrival'] = True
            state['phase'] = 3
        elif state['phase'] == 3 and age >= 2:
            position = barrels[0].get_actor_location()
            assert max(abs(getattr(position, axis)-getattr(state['arrival'], axis)) for axis in ['x','y','z']) < .01, 'Barrel drifted after arrival'
            assert rotation_values(barrels[0]) == state['arrival_rotation'], 'Barrel kept spinning after arrival'
            result['stationary_after_arrival'] = True
            state['phase'] = 4
        elif state['phase'] == 4 and age >= 4.2:
            assert not u.SystemLibrary.is_valid(barrels[0]), 'Automatic fuse failed'
            result['automatic_fuse_cleanup'] = True
            finish()
    except Exception as error:
        finish(error)

handle = u.register_slate_post_tick_callback(tick)
