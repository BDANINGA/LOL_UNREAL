"""Exercises real Actor.TakeDamage -> StatComponent, without calling the cosmetic RPC."""
import unreal as u
import json
from pathlib import Path
world = u.EditorLevelLibrary.get_pie_worlds(False)[0]
template = u.load_asset('/Game/VFX/HitImpact/P_HitImpact')
out = Path(u.Paths.project_saved_dir(), 'DesignBackups/HitIntegrationTest.json')
result = {}

def tagged(name):
    return u.GameplayStatics.get_all_actors_with_tag(world, name)[0]

def effects():
    return [c for c in world.get_world_settings().get_components_by_class(u.ParticleSystemComponent)
            if u.SystemLibrary.is_valid(c) and c.get_editor_property('template') == template]

def damage(target, amount=20.0, source=None):
    return u.GameplayStatics.apply_damage(target, amount, None, source, u.load_class(None, '/Script/Engine.DamageType'))

targets = [tagged(n) for n in ['HitChampion', 'HitMinion', 'HitBuilding']]
targets.append(u.GameplayStatics.get_all_actors_of_class(world, u.BaseJungleMonster)[0])
for actor in targets:
    actor.set_actor_hidden_in_game(False)
    stat = actor.get_component_by_class(u.LOL_StatComponent)
    data = stat.get_editor_property('base_stat')
    data.set_editor_property('armor', 0.0)
    stat.set_editor_property('base_stat', data)
    assert stat.get_editor_property('hit_impact_effect') == template
    before = len(effects())
    actual = damage(actor)
    assert actual == 20.0, (actor.get_name(), actual)
    assert len(effects()) == before + 1, 'Must spawn exactly once per accepted hit'
result['champion_minion_monster_building'] = True
result['damage_values_unchanged'] = True

champ = targets[0]
stat = champ.get_component_by_class(u.LOL_StatComponent)
before = len(effects())
assert damage(champ, 0.0) == 0.0
assert len(effects()) == before
champ.set_actor_hidden_in_game(True)
assert damage(champ) > 0
assert len(effects()) == before
champ.set_actor_hidden_in_game(False)
stat.set_editor_property('bEnableHitImpact', False)
assert damage(champ) > 0
assert len(effects()) == before
stat.set_editor_property('bEnableHitImpact', True)
result['zero_hidden_disabled_suppressed'] = True

blue = u.GameplayTag()
blue.import_text('(TagName="Team.Blue")')
attacker = tagged('HitAttacker')
champ.get_component_by_class(u.LOL_StateComponent).add_status_tag(blue)
attacker.get_component_by_class(u.LOL_StateComponent).add_status_tag(blue)
assert damage(champ, 20.0, attacker) == 0.0
assert len(effects()) == before
result['friendly_rejected_without_effect'] = True

minion = targets[1]
minion_stat = minion.get_component_by_class(u.LOL_StatComponent)
assert damage(minion, 100000.0) > 0.0
assert len(effects()) == before + 1
assert damage(minion) == 0.0 and len(effects()) == before + 1
result['lethal_hit_once_dead_target_suppressed'] = True

start = u.GameplayStatics.get_time_seconds(world)
def tick(dt):
    if u.GameplayStatics.get_time_seconds(world) - start < 1.0:
        return
    try:
        assert len(effects()) == 0, 'Auto-destroy did not clean up one-shot effects'
        result['auto_cleanup'] = True
        result['success'] = True
    except Exception as exc:
        result.update(success=False, error=str(exc))
    out.write_text(json.dumps(result, indent=2), encoding='utf8')
    u.unregister_slate_post_tick_callback(handle)
    print('HIT_INTEGRATION_RESULT', result)
handle = u.register_slate_post_tick_callback(tick)
print('HIT_INTEGRATION_RUNNING')
