"""Run via runpy.run_path after prepare_gragas_w_test.py starts its unsaved PIE fixture."""
import json
from pathlib import Path
import unreal as u

world = u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc = u.GameplayStatics.get_player_controller(world, 0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)[0], 0)
target = u.GameplayStatics.get_all_actors_with_tag(world, 'AuraExpire')[0]
target.set_actor_hidden_in_game(False)
stat = target.get_component_by_class(u.LOL_StatComponent)
floater_class = stat.get_editor_property('DamageFloaterClass')
assert floater_class and floater_class.get_path_name() == '/Game/UI/bp_damagefloater.bp_damagefloater_C'
data = stat.get_editor_property('base_stat')
data.set_editor_property('armor', 0.0)
stat.set_editor_property('base_stat', data)
damage_type = u.load_class(None, '/Script/Engine.DamageType')
assert u.GameplayStatics.apply_damage(target, 37.0, pc, None, damage_type) == 37.0
floaters = u.GameplayStatics.get_all_actors_of_class(world, floater_class)
assert len(floaters) == 1, 'Accepted damage must spawn a floater'
component = floaters[0].get_component_by_class(u.WidgetComponent)
widget = component.get_user_widget_object()
assert widget, 'Damage widget was not initialized'
tree = u.find_object(widget, 'WidgetTree') or u.find_object(widget, 'WidgetTree_0')
label = u.find_object(tree, 'text_damage')
assert label and str(label.get_text()) == '37', 'Damage amount was not assigned'
font = label.get_editor_property('font').get_editor_property('font_object')
assert font, 'Damage font is missing'
assert u.GameplayStatics.apply_damage(target, 0.0, pc, None, damage_type) == 0.0
assert len(u.GameplayStatics.get_all_actors_of_class(world, floater_class)) == 1
result = {
    'reflected_class': floater_class.get_path_name(),
    'actual_damage_text': str(label.get_text()),
    'widget': widget.get_class().get_path_name(),
    'font': font.get_path_name(),
    'zero_damage_suppressed': True,
}
out = Path(u.Paths.project_saved_dir()) / 'Validation' / 'DamageText'
out.mkdir(parents=True, exist_ok=True)
state = {'start': u.GameplayStatics.get_time_seconds(world), 'captured': False}

def tick(dt):
    try:
        age = u.GameplayStatics.get_time_seconds(world) - state['start']
        if age >= .15 and not state['captured']:
            u.SystemLibrary.execute_console_command(world,
                'HighResShot 1280x720 filename="' + (out / 'DamageText_37.png').as_posix() + '"', pc)
            state['captured'] = True
        if age < 1.6:
            return
        assert not u.GameplayStatics.get_all_actors_of_class(world, floater_class), 'Expired floater remained'
        result.update(expiration_cleanup=True, success=True)
    except Exception as error:
        result.update(success=False, error=str(error))
    else:
        if 'success' not in result:
            return
    u.unregister_slate_post_tick_callback(handle)
    (out / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf8')
    print('DAMAGE_TEXT_RESULT', result)

handle = u.register_slate_post_tick_callback(tick)
