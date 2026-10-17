"""Run via runpy.run_path in PIE: real recall RPCs, cancellation, damage, fog and completion."""
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
casters = [u.GameplayStatics.get_all_actors_with_tag(world, tag)[0]
           for tag in ['RecallComplete', 'RecallCancel', 'RecallDamage', 'RecallDeath']]
for index, caster in enumerate(casters):
    caster.set_actor_hidden_in_game(False)
    caster.call_method('Server_CancelRecall', ())
    status = caster.get_component_by_class(u.LOL_StateComponent)
    for name in ['Team.Blue', 'Team.Red', 'State.Dead']:
        tag = u.GameplayTag()
        tag.import_text('(TagName="' + name + '")')
        status.remove_status_tag(tag)
    tag = u.GameplayTag()
    tag.import_text('(TagName="' + ('Team.Red' if index == 2 else 'Team.Blue') + '")')
    status.add_status_tag(tag)

out = Path(u.Paths.project_saved_dir()) / 'Validation' / 'Recall'
out.mkdir(parents=True, exist_ok=True)
result = {}
state = {'phase': 0, 'start': u.GameplayStatics.get_time_seconds(world)}
home = casters[0].get_actor_location()
casters[0].set_actor_location(home + u.Vector(360, 0, 0), False, False)
camera = u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)[0]
camera.set_actor_location(u.Vector(960, -850, 950), False, False)

def layers(actor):
    return [c for c in actor.get_components_by_class(u.StaticMeshComponent)
            if isinstance(c.get_material(0), u.MaterialInstanceDynamic)
            and '/Game/VFX/Recall/' in c.get_material(0).get_editor_property('parent').get_path_name()]

def capture(name):
    u.SystemLibrary.execute_console_command(world, 'HighResShot 1280x720 filename="' + (out / name).as_posix() + '"', pc)

def finish(error=None):
    result['success'] = error is None
    if error:
        result['error'] = str(error)
    u.unregister_slate_post_tick_callback(handle)
    (out / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print('RECALL_TEST_RESULT', result)

for caster in casters:
    caster.call_method('Server_StartRecall', ())
    assert len(layers(caster)) == 13, 'Missing recall layers'
    caster.call_method('Server_StartRecall', ())
    assert len(layers(caster)) == 13, 'Repeated recall duplicated layers'
result['starts_and_rejects_duplicate'] = True

def tick(dt):
    try:
        age = u.GameplayStatics.get_time_seconds(world) - state['start']
        phase = state['phase']
        if phase == 0 and age > 1.2:
            capture('Recall_Early.png')
            casters[1].call_method('Server_ProcessMoveInput', (casters[1].get_actor_location() + u.Vector(300, 0, 0), None, False))
            u.GameplayStatics.apply_damage(casters[2], 25, pc, casters[0], u.load_class(None, '/Script/Engine.DamageType'))
            dead = u.GameplayTag()
            dead.import_text('(TagName="State.Dead")')
            casters[3].get_component_by_class(u.LOL_StateComponent).add_status_tag(dead)
            state['phase'] = 1
        elif phase == 1 and age > 1.7:
            assert not layers(casters[1]), 'Movement cancellation left visuals'
            assert not layers(casters[2]), 'Damage cancellation left visuals'
            assert not layers(casters[3]), 'Dead champion retained visuals'
            result.update(movement_cancel=True, damage_cancel=True, death_cleanup=True)
            casters[1].call_method('Server_StartRecall', ())
            assert len(layers(casters[1])) == 13
            casters[1].call_method('Server_CancelRecall', ())
            casters[1].call_method('Server_StartRecall', ())
            assert len(layers(casters[1])) == 13, 'Restart during fade duplicated visuals'
            casters[1].call_method('Server_CancelRecall', ())
            result['restart_during_fade'] = True
            casters[0].set_actor_hidden_in_game(True)
            state['phase'] = 2
        elif phase == 2 and age > 2.0:
            assert all(not c.is_visible() for c in layers(casters[0])), 'Hidden champion revealed recall'
            casters[0].set_actor_hidden_in_game(False)
            result['fog_hidden'] = True
            state['phase'] = 3
        elif phase == 3 and age > 3.5:
            assert any(c.is_visible() for c in layers(casters[0]))
            capture('Recall_Channel.png')
            state['phase'] = 4
        elif phase == 4 and age > 7.1:
            capture('Recall_Charged.png')
            assert (casters[0].get_actor_location() - home).length() > 300
            result['no_early_teleport'] = True
            state['phase'] = 5
        elif phase == 5 and age > 8.7:
            assert (casters[0].get_actor_location() - home).length() < 10, 'Recall did not return home'
            assert not layers(casters[0]), 'Completed recall retained visuals'
            assert not casters[0].get_component_by_class(u.LOL_RecallVisualComponent).is_component_tick_enabled()
            result.update(eight_second_recall=True, completion_cleanup=True, idle_tick_disabled=True)
            finish()
    except Exception as error:
        finish(error)

handle = u.register_slate_post_tick_callback(tick)
