"""Actual W: particles, following/visibility, expiration, empowered hit and death cleanup."""
import unreal as u
import json
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn:
    pawn.set_actor_hidden_in_game(True)
    pawn.set_actor_enable_collision(False)
casters=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in ['AuraExpire','AuraAttack','AuraDeath']]
victim=u.GameplayStatics.get_all_actors_with_tag(world,'AuraVictim')[0]
for actor,team in [(a,'Team.Blue') for a in casters]+[(victim,'Team.Red')]:
    tag=u.GameplayTag(); tag.import_text('(TagName="'+team+'")')
    actor.get_component_by_class(u.LOL_StateComponent).add_status_tag(tag)
    actor.set_actor_hidden_in_game(False)
template=u.load_asset('/Game/VFX/Gragas/P_GragasWAura')
assert template
def auras(actor):
    return [c for c in actor.get_components_by_class(u.ParticleSystemComponent) if u.SystemLibrary.is_valid(c) and c.get_editor_property('template')==template]
for caster in casters:
    caster.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('W',))
    caster.call_method('Server_Skill_W',())
    assert len(auras(caster))==1, 'W did not spawn one aura'
    caster.call_method('Server_Skill_W',())
    assert len(auras(caster))==1, 'Rejected cast duplicated the aura'
out=Path(u.Paths.project_saved_dir(),'DesignBackups');out.mkdir(parents=True,exist_ok=True)
result={'actual_cast_and_cooldown':True}
state={'phase':0,'start':u.GameplayStatics.get_time_seconds(world),'last_attack':0,'attack_seen':False}
u.GameplayStatics.set_global_time_dilation(world,.6)

def finish(error=None):
    result['success']=error is None
    if error: result['error']=str(error)
    u.GameplayStatics.set_global_time_dilation(world,1)
    u.unregister_slate_post_tick_callback(handle)
    (out/'GragasWAuraTest.json').write_text(json.dumps(result,indent=2),encoding='utf8')
    print('GRAGAS_W_AURA_RESULT',result)

def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-state['start']
        if state['phase']==0 and age>=.6:
            aura=auras(casters[0])[0]
            result['active_particles']=aura.get_num_active_particles()
            assert result['active_particles']>10
            u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/'GragasW_Aura.png').as_posix()+'"',pc)
            state['phase']=1
        elif state['phase']==1 and age>=.9:
            caster=casters[0]
            caster.set_actor_location(caster.get_actor_location()+u.Vector(150,0,0),False,False)
            assert (auras(caster)[0].get_world_location()-(caster.get_actor_location()+u.Vector(0,0,10))).length()<.1
            caster.set_actor_hidden_in_game(True)
            state['phase']=2
        elif state['phase']==2 and age>=1.1:
            assert not auras(casters[0])[0].is_visible()
            casters[0].set_actor_hidden_in_game(False)
            dead=u.GameplayTag();dead.import_text('(TagName="State.Dead")')
            casters[2].get_component_by_class(u.LOL_StateComponent).add_status_tag(dead)
            state['phase']=3
        elif state['phase']==3 and age>=1.3:
            assert auras(casters[0])[0].is_visible()
            assert not auras(casters[2]), 'Dead champion retained aura'
            result.update(follows_movement=True,fog_visibility=True,death_cleanup=True)
            state['phase']=4
        if 2<age<4.8 and not state['attack_seen']:
            if age-state['last_attack']>.2:
                casters[1].call_method('Server_ProcessMoveInput',(victim.get_actor_location(),victim,False))
                state['last_attack']=age
            if not auras(casters[1]):
                result['empowered_attack_cleanup']=True
                state['attack_seen']=True
        if age>5.3:
            assert state['attack_seen'], 'Empowered attack did not consume aura before timeout'
            assert not any(auras(c) for c in casters)
            result['expiration_cleanup']=True
            finish()
    except Exception as error:
        finish(error)

handle=u.register_slate_post_tick_callback(tick)
