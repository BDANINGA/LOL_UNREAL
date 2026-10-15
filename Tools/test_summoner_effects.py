"""Actual server spell paths: success/rejection, duration, tracking, visibility and cleanup."""
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
caster, target, ally = [u.GameplayStatics.get_all_actors_with_tag(world, n)[0] for n in ['SpellCaster', 'SpellTarget', 'SpellAlly']]
for actor, team in [(caster, 'Team.Blue'), (target, 'Team.Red'), (ally, 'Team.Blue')]:
    tag = u.GameplayTag(); tag.import_text('(TagName="' + team + '")')
    actor.get_component_by_class(u.LOL_StateComponent).add_status_tag(tag)
    actor.set_actor_hidden_in_game(False)
spell, target_spell, ally_spell = [a.get_component_by_class(u.LOL_SummonerSpellComponent) for a in [caster, target, ally]]
ally_stat = ally.get_component_by_class(u.LOL_StatComponent)
ally_data = ally_stat.get_editor_property('base_stat')
ally_data.set_editor_property('armor', 0.0)
ally_stat.set_editor_property('base_stat', ally_data)
flash_templates = [u.load_asset('/Game/VFX/SummonerSpells/' + n) for n in ['P_FlashDeparture', 'P_FlashArrival']]
aura_template = u.load_asset('/Game/VFX/SummonerSpells/P_ExhaustAura')
assert all(flash_templates) and aura_template

def flashes():
    return [c for c in world.get_world_settings().get_components_by_class(u.ParticleSystemComponent)
            if u.SystemLibrary.is_valid(c) and c.get_editor_property('template') in flash_templates]

def auras(actor=target):
    return [c for c in actor.get_components_by_class(u.ParticleSystemComponent)
            if u.SystemLibrary.is_valid(c) and c.get_editor_property('template') == aura_template]

def flash(where):
    spell.call_method('Server_CastFlash', (net_vector(where),))

def net_vector(where):
    value = u.Vector_NetQuantize()
    for axis in ['x', 'y', 'z']:
        value.set_editor_property(axis, getattr(where, axis))
    return value

def exhaust(component=spell, victim=target):
    component.call_method('Server_CastExhaust', (victim,))

def capture(name):
    u.SystemLibrary.execute_console_command(world, 'HighResShot 1280x720 filename="' + (out/name).as_posix() + '"', pc)

out = Path(u.Paths.project_saved_dir(), 'DesignBackups'); out.mkdir(parents=True, exist_ok=True)
result = {}
origin = caster.get_actor_location()
flash(origin)
assert caster.get_actor_location().equals(origin) and not flashes()
u.GameplayStatics.set_global_time_dilation(world, .25)
flash(origin + u.Vector(900, 0, 0))
arrival = caster.get_actor_location()
distance_2d = ((arrival.x-origin.x)**2 + (arrival.y-origin.y)**2)**.5
assert 1 < distance_2d <= 400.01, distance_2d
assert len(flashes()) == 2
for template, location in zip(flash_templates, [origin, arrival]):
    assert next(c for c in flashes() if c.get_editor_property('template') == template).get_world_location().equals(location)
flash(origin)
assert caster.get_actor_location().equals(arrival) and len(flashes()) == 2
result.update(flash_actual_cast=True, flash_range_and_endpoints=True, flash_zero_distance_and_cooldown_rejected=True)
state = {'phase': 0, 'start': u.GameplayStatics.get_time_seconds(world)}
base_speed = target.character_movement.max_walk_speed

def finish():
    u.GameplayStatics.set_global_time_dilation(world, 1)
    (out/'SummonerEffectsTest.json').write_text(json.dumps(result, indent=2), encoding='utf8')
    u.unregister_slate_post_tick_callback(handle)
    print('SUMMONER_EFFECTS_RESULT', result)

def tick(dt):
    try:
        now = u.GameplayStatics.get_time_seconds(world)
        age = now - state['start']
        phase = state['phase']
        if phase == 0 and age >= .055:
            counts = [c.get_num_active_particles() for c in flashes()]
            assert len(counts) == 2 and min(counts) > 0
            result['flash_particles'] = counts
            capture('Flash_Preview.png')
            state['phase'] = 1
        elif phase == 1 and age >= .7:
            assert not flashes(), 'Flash particles did not auto-destroy'
            result['flash_cleanup'] = True
            exhaust(victim=caster); exhaust(victim=ally)
            assert not auras() and not auras(caster) and not auras(ally)
            old = target.get_actor_location()
            target.set_actor_location(u.Vector(3000, 0, 100), False, False)
            exhaust(); assert not auras()
            target.set_actor_location(old, False, False)
            exhaust()
            assert len(auras()) == 1
            assert abs(target.character_movement.max_walk_speed - base_speed*.7) < .01
            # Damage from the exhausted champion still follows the original multiplier.
            dealt = u.GameplayStatics.apply_damage(ally, 100, None, target, u.LOL_DamagePhysical)
            assert abs(dealt - 65) < .01, dealt
            aura = auras()[0]
            exhaust(); assert auras() == [aura]
            result.update(exhaust_actual_cast=True, exhaust_rejections=True, gameplay_multipliers_preserved=True)
            state.update(phase=2, start=now, aura=aura)
        elif phase == 2 and age >= .8:
            aura = state['aura']
            assert aura.get_num_active_particles() > 0
            result['exhaust_particles'] = aura.get_num_active_particles()
            capture('Exhaust_Preview.png')
            state.update(phase=7, captured_at=now)
        elif phase == 7 and now - state['captured_at'] >= .2:
            aura = state['aura']
            target.set_actor_location(u.Vector(380, 0, 100), False, False)
            assert aura.get_world_location().equals(target.get_actor_location())
            target.set_actor_hidden_in_game(True)
            state.update(phase=3, hidden_at=now)
        elif phase == 3 and now - state['hidden_at'] >= .15:
            assert not state['aura'].is_visible(), 'Hidden target revealed by aura'
            target.set_actor_hidden_in_game(False)
            state.update(phase=4, visible_at=now)
        elif phase == 4 and now - state['visible_at'] >= .15:
            assert state['aura'].is_visible()
            ally_spell.set_editor_property('exhaust_duration', 1.0)
            ally_spell.set_editor_property('exhaust_cooldown', 0.0)
            exhaust(ally_spell)
            assert auras() == [state['aura']], 'Refresh stacked aura components'
            result.update(exhaust_follows_target=True, hidden_visibility=True, refresh_reuses_aura=True)
            state.update(phase=5, start=now)
        elif phase == 5 and age >= 1.2:
            assert not auras(), 'Exhaust aura outlived refreshed duration'
            assert abs(target.character_movement.max_walk_speed-base_speed)<.01
            dealt = u.GameplayStatics.apply_damage(ally, 100, None, target, u.LOL_DamagePhysical)
            assert abs(dealt-100)<.01
            result['exhaust_expiry_restores_and_cleans'] = True
            exhaust(ally_spell)
            assert len(auras()) == 1
            u.GameplayStatics.apply_damage(target, 100000, None, caster, u.LOL_DamageTrueDamage)
            state.update(phase=6, start=now)
        elif phase == 6 and age >= .15:
            assert not auras(), 'Dead target retained aura'
            exhaust(ally_spell); assert not auras()
            # Cosmetic toggle and fog suppression do not spawn Flash effects.
            spell.set_editor_property('bEnableSpellEffects', False)
            spell.call_method('Multicast_PlayFlashEffect', (net_vector(origin), net_vector(arrival))); assert not flashes()
            spell.set_editor_property('bEnableSpellEffects', True)
            caster.set_actor_hidden_in_game(True)
            spell.call_method('Multicast_PlayFlashEffect', (net_vector(origin), net_vector(arrival))); assert not flashes()
            caster.set_actor_hidden_in_game(False)
            result.update(death_cleanup=True, flash_disabled_hidden_suppressed=True, success=True)
            finish()
    except Exception as exc:
        result.update(success=False, error=str(exc)); finish()

handle = u.register_slate_post_tick_callback(tick)
print('SUMMONER_EFFECTS_TEST_RUNNING')
