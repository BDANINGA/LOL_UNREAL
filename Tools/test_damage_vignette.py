"""Actual TakeDamage integration: ownership, rejected hits, pulse/fade, reuse and death."""
import unreal as u
import json
from pathlib import Path

world = u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc = u.GameplayStatics.get_player_controller(world, 0)
assert isinstance(pc, u.LOL_PlayerController)
victim = u.GameplayStatics.get_all_actors_with_tag(world, 'VignetteVictim')[0]
other = u.GameplayStatics.get_all_actors_with_tag(world, 'VignetteOther')[0]
pc.possess(victim)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)[0], 0)
out = Path(u.Paths.project_saved_dir(), 'DesignBackups')
out.mkdir(parents=True, exist_ok=True)
result = {}

def widgets():
    return u.WidgetLibrary.get_all_widgets_of_class(world, u.LOL_DamageVignetteWidget, False)

def damage(target, amount, source=None):
    return u.GameplayStatics.apply_damage(target, amount, None, source, u.LOL_DamageTrueDamage)

assert not widgets()
assert damage(other, 20) == 20 and not widgets()
assert damage(victim, 0) == 0 and not widgets()
assert damage(victim, -20) == 0 and not widgets()
tag = u.GameplayTag(); tag.import_text('(TagName="Team.Blue")')
for actor in [victim, other]:
    actor.get_component_by_class(u.LOL_StateComponent).add_status_tag(tag)
assert damage(victim, 20, other) == 0 and not widgets()
result['other_target_zero_negative_friendly_no_flash'] = True
assert damage(victim, 80) == 80
assert len(widgets()) == 1
widget = widgets()[0]
assert widget.get_owning_player() == pc
assert widget.get_visibility() == u.SlateVisibility.HIT_TEST_INVISIBLE
assert not widget.get_editor_property('bIsFocusable')
state = {'phase': 0, 'start': u.GameplayStatics.get_real_time_seconds(world)}

def finish():
    (out / 'DamageVignetteTest.json').write_text(json.dumps(result, indent=2), encoding='utf8')
    u.unregister_slate_post_tick_callback(handle)
    print('DAMAGE_VIGNETTE_TEST_RESULT', result)

def tick(dt):
    try:
        now = u.GameplayStatics.get_real_time_seconds(world)
        age = now - state['start']
        phase = state['phase']
        if phase == 0 and age >= .09:
            alpha = widget.get_flash_opacity()
            assert .15 < alpha <= .65, ('No visible pulse', alpha, age)
            state['peak'] = alpha
            result['local_damage_visible'] = True
            # Capture separately: synchronous Slate screenshots can outlast the pulse.
            state['phase'] = 1
        elif phase == 1 and age >= .30:
            assert 0 < widget.get_flash_opacity() < state['peak'], 'Pulse did not fade'
            result['smooth_fade'] = True
            for _ in range(20):
                assert damage(victim, 1) == 1
            assert len(widgets()) == 1 and widgets()[0] == widget
            state.update(phase=2, start=now)
        elif phase == 2 and age >= .1:
            assert 0 < widget.get_flash_opacity() <= .65
            result['repeated_hits_reuse_and_cap'] = True
            state['phase'] = 3
        elif phase == 3 and age >= .75:
            assert widget.get_flash_opacity() == 0 and widget.get_visibility() == u.SlateVisibility.HIDDEN
            pc.set_editor_property('bEnableDamageVignette', False)
            assert damage(victim, 20) == 20
            assert widget.get_visibility() == u.SlateVisibility.HIDDEN
            pc.set_editor_property('bEnableDamageVignette', True)
            pc.call_method('Client_PlayDamageVignette', (other, .5))
            assert widget.get_visibility() == u.SlateVisibility.HIDDEN
            result['fade_complete_disabled_wrong_pawn'] = True
            assert damage(victim, 20) == 20
            pc.possess(other)
            state.update(phase=4, start=now)
        elif phase == 4 and age >= .12:
            assert widget.get_flash_opacity() == 0 and widget.get_visibility() == u.SlateVisibility.HIDDEN
            result['possession_change_clears'] = True
            pc.possess(victim)
            assert damage(victim, 100000) > 0
            assert widget.get_visibility() == u.SlateVisibility.HIT_TEST_INVISIBLE
            assert damage(victim, 20) == 0
            result['lethal_hit_and_dead_target'] = True
            state.update(phase=5, start=now)
        elif phase == 5 and age >= .75:
            assert widget.get_flash_opacity() == 0
            assert len(widgets()) == 1
            result['success'] = True
            finish()
    except Exception as exc:
        result.update(success=False, error=str(exc))
        finish()

handle = u.register_slate_post_tick_callback(tick)
print('DAMAGE_VIGNETTE_TEST_RUNNING')
