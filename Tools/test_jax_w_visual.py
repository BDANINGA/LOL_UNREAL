"""Run using runpy.run_path after prepare_jax_w_test.py starts PIE."""
import json
from pathlib import Path
import unreal as u

world = u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc = u.GameplayStatics.get_player_controller(world, 0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)[0], 0)
pawn = u.GameplayStatics.get_player_pawn(world, 0)
if pawn:
    pawn.set_actor_hidden_in_game(True)
    pawn.set_actor_enable_collision(False)
casters = [u.GameplayStatics.get_all_actors_with_tag(world, name)[0] for name in
           ['EmpowerExpire', 'EmpowerAttack', 'EmpowerDeath', 'EmpowerQ', 'EmpowerUnlearned']]
victims = [u.GameplayStatics.get_all_actors_with_tag(world, name)[0] for name in ['AttackVictim', 'QVictim']]
for actor, team in [(a, 'Team.Blue') for a in casters] + [(a, 'Team.Red') for a in victims]:
    status = actor.get_component_by_class(u.LOL_StateComponent)
    for name in ['Team.Blue', 'Team.Red', 'State.Dead']:
        tag = u.GameplayTag(); tag.import_text('(TagName="' + name + '")')
        status.remove_status_tag(tag)
    tag = u.GameplayTag(); tag.import_text('(TagName="' + team + '")')
    status.add_status_tag(tag)
    actor.set_actor_hidden_in_game(False)
originals = [a.mesh.get_material(1) for a in casters]
bodies = [a.mesh.get_material(0) for a in casters]
halos = [a.get_component_by_class(u.MaterialBillboardComponent) for a in casters]
out = Path(u.Paths.project_saved_dir()) / 'Validation' / 'JaxW'
out.mkdir(parents=True, exist_ok=True)
result = {}
state = {'phase': 0, 'start': u.GameplayStatics.get_time_seconds(world), 'attack_seen': False, 'last_attack': 0}

def active(actor):
    material = actor.mesh.get_material(1)
    return isinstance(material, u.MaterialInstanceDynamic) and '/Game/VFX/Jax/M_JaxEmpowerWeapon' in material.parent.get_path_name()

def capture(name):
    u.SystemLibrary.execute_console_command(world, 'HighResShot 1280x720 filename="' + (out / name).as_posix() + '"', pc)

def finish(error=None):
    result['success'] = error is None
    if error: result['error'] = str(error)
    u.unregister_slate_post_tick_callback(handle)
    (out / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf8')
    print('JAX_W_TEST_RESULT', result)

casters[4].call_method('Server_Skill_W', ())
assert not active(casters[4]), 'Unlearned W started a glow'
result['unlearned_rejected'] = True
for caster in casters[:4]:
    skill = caster.get_component_by_class(u.Champion_SkillComponent)
    skill.call_method('Server_LevelUpSkill', ('W',))
    caster.call_method('Server_Skill_W', ())
    assert active(caster), 'Actual W did not light the weapon'
    material = caster.mesh.get_material(1)
    caster.call_method('Server_Skill_W', ())
    assert caster.mesh.get_material(1) == material, 'Rejected recast replaced material'
assert all(a.mesh.get_material(0) == b for a, b in zip(casters, bodies))
assert all(h.is_visible() for h in halos[:4]) and not halos[4].is_visible()
assert all(str(h.get_attach_socket_name()) == 'Buffbone_Glb_Weapon_Loc' for h in halos)
result.update(actual_cast=True, cooldown_rejected=True, body_unchanged=True)
# Use the existing debug level-up RPC to obtain a second skill point.
qskill = casters[3].get_component_by_class(u.Champion_SkillComponent)
u.SystemLibrary.execute_console_command(world, 'summon /Script/LOL_UNREAL.LOL_PlayerController', pc)
debug_pc = u.GameplayStatics.get_all_actors_of_class(world, u.LOL_PlayerController)[-1]
debug_pc.possess(casters[3])
debug_pc.call_method('Server_DebugLevelUp', ())
debug_pc.set_actor_tick_enabled(False)
qskill.call_method('Server_LevelUpSkill', ('Q',))

def tick(dt):
    try:
        age = u.GameplayStatics.get_time_seconds(world) - state['start']
        if state['phase'] == 0 and age > .6:
            capture('Jax_W_Active.png')
            state['phase'] = 1
        elif state['phase'] == 1 and age > 1:
            dead = u.GameplayTag(); dead.import_text('(TagName="State.Dead")')
            casters[2].get_component_by_class(u.LOL_StateComponent).add_status_tag(dead)
            casters[3].call_method('Server_Skill_Q', (victims[1],))
            casters[0].set_actor_hidden_in_game(True)
            state['phase'] = 2
        elif state['phase'] == 2 and age > 1.6:
            assert not active(casters[2]), 'Death retained glow'
            assert not active(casters[3]), 'Q failed to consume W'
            assert not halos[2].is_visible() and not halos[3].is_visible()
            assert (halos[0].get_world_location() - casters[0].mesh.get_socket_location('Buffbone_Glb_Weapon_Loc')).length() < .1
            assert casters[0].hidden and active(casters[0])
            casters[0].set_actor_hidden_in_game(False)
            result.update(death_cleanup=True, q_consumption=True, fog_uses_owner_mesh=True, halo_follows_weapon=True)
            state['phase'] = 3
        if 1.7 < age < 4.8 and not state['attack_seen']:
            if age - state['last_attack'] > .2:
                casters[1].call_method('Server_ProcessMoveInput', (victims[0].get_actor_location(), victims[0], False))
                state['last_attack'] = age
            if not active(casters[1]):
                state['attack_seen'] = True
                result['basic_attack_consumption'] = True
        if age > 5.4:
            assert state['attack_seen'], 'Basic attack did not consume W before timeout'
            assert all(a.mesh.get_material(1) == m for a, m in zip(casters, originals)), 'Original material not restored'
            assert not any(h.is_visible() for h in halos), 'Halo remained after W'
            result['expiration_and_exact_restore'] = True
            capture('Jax_W_Off.png')
            finish()
    except Exception as error:
        finish(error)

handle = u.register_slate_post_tick_callback(tick)
