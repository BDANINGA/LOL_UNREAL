"""Real Q cast: particles, damage, repeated-cast rejection, fog and cleanup."""
import unreal as u
import json
import re
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn: pawn.set_actor_hidden_in_game(True);pawn.set_actor_enable_collision(False)
names=['Complete','Hidden','Death','LostTarget','Unlearned']
casters=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in names]
targets=[u.GameplayStatics.get_all_actors_with_tag(world,n+'Target')[0] for n in names]
def tag(name):
    t=u.GameplayTag();t.import_text('(TagName="'+name+'")');return t
for actor,team in [(a,'Team.Blue') for a in casters]+[(a,'Team.Red') for a in targets]:
    status=actor.get_component_by_class(u.LOL_StateComponent)
    for n in ['State.Dead','Team.Blue','Team.Red']:status.remove_status_tag(tag(n))
    status.add_status_tag(tag(team));actor.set_actor_hidden_in_game(False)
    actor.character_movement.set_movement_mode(u.MovementMode.MOVE_NONE)
    pos=actor.get_actor_location();pos.z=actor.capsule_component.get_scaled_capsule_half_height()+2
    actor.set_actor_location(pos,False,False)
template=u.load_asset('/Game/VFX/Fizz/P_FizzQWater')
assert template
def effects(actor):
    return [c for c in actor.get_components_by_class(u.ParticleSystemComponent) if u.SystemLibrary.is_valid(c) and c.get_editor_property('template')==template]
out=Path(u.Paths.project_saved_dir())/'Validation/FizzQ';out.mkdir(parents=True,exist_ok=True)
def hp(actor):
    task=u.AssetExportTask();task.object=actor.get_component_by_class(u.LOL_StatComponent)
    task.filename=str(out/'stat.t3d');task.automated=True;task.prompt=False;task.replace_identical=True
    task.exporter=u.ObjectExporterT3D();assert u.Exporter.run_asset_export_task(task)
    raw=(out/'stat.t3d').read_bytes()
    text=raw.decode('utf-16') if raw.startswith(b'\xff\xfe') else raw.decode('utf-8-sig')
    return float(re.search(r'CurrentHP=([\d.]+)',text).group(1))
before=hp(targets[0])
start=casters[0].get_actor_location()
casters[4].call_method('Server_Skill_Q',(targets[4],));assert not effects(casters[4])
for caster,target in zip(casters[:4],targets[:4]):
    caster.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('Q',))
    caster.call_method('Server_Skill_Q',(target,));assert len(effects(caster))==1
    caster.call_method('Server_Skill_Q',(target,));assert len(effects(caster))==1
u.GameplayStatics.set_global_time_dilation(world,.15)
result={'actual_cast':True,'unlearned_rejected':True,'duplicate_rejected':True}
state={'phase':0,'start':u.GameplayStatics.get_time_seconds(world)}
def capture(name):
    u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/name).as_posix()+'"',pc)
def finish(error=None):
    result['success']=error is None
    if error:result['error']=repr(error)
    u.GameplayStatics.set_global_time_dilation(world,1)
    u.unregister_slate_post_tick_callback(handle)
    (out/'result.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('FIZZ_Q_RESULT',result)
def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-state['start'];phase=state['phase']
        if phase==0 and age>=.1:
            result['particles']=effects(casters[0])[0].get_num_active_particles();assert result['particles']>10
            assert casters[0].get_actor_location().x>start.x+40
            casters[1].set_actor_hidden_in_game(True)
            casters[2].get_component_by_class(u.LOL_StateComponent).add_status_tag(tag('State.Dead'))
            targets[3].destroy_actor()
            capture('FizzQ_Dash.png');state['phase']=1
        elif phase==1 and age>=.23:
            assert not effects(casters[1])[0].is_visible();casters[1].set_actor_hidden_in_game(False)
            assert not effects(casters[2])
            result.update(fog_hidden=True,death_cleanup=True)
            capture('FizzQ_Spray.png');state['phase']=2
        elif phase==2 and age>=.32:
            assert effects(casters[1])[0].is_visible();result['fog_restored']=True;state['phase']=3
        elif phase==3 and age>=.56:
            after=hp(targets[0])
            assert after<before
            assert casters[0].get_actor_location().x>targets[0].get_actor_location().x
            assert effects(casters[0])
            result.update(damage=before-after,passed_through_target=True,residual_particles=effects(casters[0])[0].get_num_active_particles())
            capture('FizzQ_Fade.png');state['phase']=4
        elif phase==4 and age>=1.2:
            assert not any(effects(c) for c in casters)
            result.update(fade_cleanup=True,lost_target_cleanup=True);finish()
    except Exception as error:finish(error)
handle=u.register_slate_post_tick_callback(tick)
