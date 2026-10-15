"""Real E casts: dash particles, impact, fading, fog visibility and death cleanup."""
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
casters=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in ['DashClear','DashHit','DashHidden','DashDeath']]
victim=u.GameplayStatics.get_all_actors_with_tag(world,'DashVictim')[0]
for actor,team in [(a,'Team.Blue') for a in casters]+[(victim,'Team.Red')]:
    tag=u.GameplayTag();tag.import_text('(TagName="'+team+'")')
    actor.get_component_by_class(u.LOL_StateComponent).add_status_tag(tag)
    actor.set_actor_hidden_in_game(False)
template=u.load_asset('/Game/VFX/Gragas/P_GragasEDashAura')
assert template
def auras(actor):
    return [c for c in actor.get_components_by_class(u.ParticleSystemComponent) if u.SystemLibrary.is_valid(c) and c.get_editor_property('template')==template]
origins=[a.get_actor_location() for a in casters]
for caster in casters:
    caster.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('E',))
    caster.call_method('Server_Skill_E',(caster.get_actor_location()+u.Vector(600,0,0),))
    assert len(auras(caster))==1
    caster.call_method('Server_Skill_E',(caster.get_actor_location()+u.Vector(600,0,0),))
    assert len(auras(caster))==1, 'Repeated E duplicated particles'
out=Path(u.Paths.project_saved_dir(),'DesignBackups');out.mkdir(parents=True,exist_ok=True)
result={'actual_cast_and_rejection':True}
state={'phase':0,'start':u.GameplayStatics.get_time_seconds(world),'hit_seen':False}
floater=u.load_class(None,'/Game/UI/bp_damagefloater.bp_damagefloater_C')
u.GameplayStatics.set_global_time_dilation(world,.1)
def capture(name):
    u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/name).as_posix()+'"',pc)
def finish(error=None):
    result['success']=error is None
    if error:result['error']=str(error)
    u.GameplayStatics.set_global_time_dilation(world,1)
    u.unregister_slate_post_tick_callback(handle)
    (out/'GragasEAuraTest.json').write_text(json.dumps(result,indent=2),encoding='utf8')
    print('GRAGAS_E_AURA_RESULT',result)
def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-state['start']
        if not state['hit_seen'] and u.GameplayStatics.get_all_actors_of_class(world,floater):
            state['hit_seen']=True
            result['impact_at']=age
            result['impact_travel']=casters[1].get_actor_location().x-origins[1].x
        phase=state['phase']
        if phase==0 and age>=.08:
            result['active_particles']=auras(casters[0])[0].get_num_active_particles()
            assert result['active_particles']>10
            assert casters[0].get_actor_location().x>origins[0].x+50
            casters[2].set_actor_hidden_in_game(True)
            state['phase']=1
        elif phase==1 and age>=.12:
            assert not auras(casters[2])[0].is_visible()
            casters[2].set_actor_hidden_in_game(False)
            dead=u.GameplayTag();dead.import_text('(TagName="State.Dead")')
            casters[3].get_component_by_class(u.LOL_StateComponent).add_status_tag(dead)
            capture('GragasE_Dash_0.png')
            state['phase']=2
        elif phase==2 and age>=.22:
            assert auras(casters[2])[0].is_visible()
            assert not auras(casters[3])
            result.update(fog_visibility=True,death_cleanup=True)
            capture('GragasE_Dash_1.png')
            state['phase']=3
        elif phase==3 and age>=.4:
            assert abs(casters[0].get_actor_location().x-origins[0].x-600)<1
            assert state['hit_seen'] and result['impact_travel']<590
            assert auras(casters[0]) and auras(casters[0])[0].get_num_active_particles()>0
            result['residual_particles_after_dash']=auras(casters[0])[0].get_num_active_particles()
            capture('GragasE_Fade.png')
            state['phase']=4
        elif phase==4 and age>=.9:
            assert not any(auras(c) for c in casters)
            result.update(dash_and_hit_preserved=True,fade_cleanup=True)
            finish()
    except Exception as error:finish(error)
handle=u.register_slate_post_tick_callback(tick)
