"""Run after prepare_fizz_w_test.py. Uses real W and basic-attack input paths."""
import unreal as u
import json,re,traceback
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn:pawn.set_actor_hidden_in_game(True);pawn.set_actor_enable_collision(False)
names=['Attack','Expire','Death','Hidden','Q','Unlearned']
casters=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in names]
targets=[u.GameplayStatics.get_all_actors_with_tag(world,n+'Target')[0] for n in names]
out=Path(u.Paths.project_saved_dir())/'Validation/FizzW';out.mkdir(parents=True,exist_ok=True)
def tag(name):
    t=u.GameplayTag();t.import_text('(TagName="'+name+'")');return t
for actor,team in [(a,'Team.Blue') for a in casters]+[(a,'Team.Red') for a in targets]:
    status=actor.get_component_by_class(u.LOL_StateComponent)
    for n in ['State.Dead','Team.Blue','Team.Red']:status.remove_status_tag(tag(n))
    status.add_status_tag(tag(team));actor.set_actor_hidden_in_game(False)
    actor.character_movement.set_movement_mode(u.MovementMode.MOVE_NONE)
    pos=actor.get_actor_location();pos.z=actor.capsule_component.get_scaled_capsule_half_height()+2
    actor.set_actor_location(pos,False,False)
weapon=u.load_asset('/Game/VFX/Fizz/P_FizzWWeapon');impact=u.load_asset('/Game/VFX/Fizz/P_FizzWImpact')
def active(actor):
    return [c for c in actor.get_components_by_class(u.ParticleSystemComponent) if u.SystemLibrary.is_valid(c) and c.get_editor_property('template')==weapon]
def impacts():
    return [c for c in world.get_world_settings().get_components_by_class(u.ParticleSystemComponent) if u.SystemLibrary.is_valid(c) and c.get_editor_property('template')==impact and c.get_num_active_particles()>0]
def hp(actor):
    task=u.AssetExportTask();task.object=actor.get_component_by_class(u.LOL_StatComponent)
    task.filename=str(out/'stat.t3d');task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=u.ObjectExporterT3D()
    assert u.Exporter.run_asset_export_task(task)
    raw=(out/'stat.t3d').read_bytes();s=raw.decode('utf-16') if raw.startswith(b'\xff\xfe') else raw.decode('utf-8-sig')
    return float(re.search(r'CurrentHP=([\d.]+)',s).group(1))
originals=[a.mesh.get_material(0) for a in casters]
casters[5].call_method('Server_Skill_W',());assert not active(casters[5])
# Use the project's debug level-up path to learn both Q and W in this fixture.
u.SystemLibrary.execute_console_command(world,'summon /Script/LOL_UNREAL.LOL_PlayerController',pc)
debug_pc=u.GameplayStatics.get_all_actors_of_class(world,u.LOL_PlayerController)[-1]
debug_pc.possess(casters[4]);debug_pc.call_method('Server_SetDebugMode',(True,))
debug_pc.call_method('Server_DebugLevelUp',());debug_pc.call_method('Server_SetDebugMode',(False,));debug_pc.set_actor_tick_enabled(False)
for caster in casters[:5]:
    caster.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('W',))
    caster.call_method('Server_Skill_W',());assert len(active(caster))==1
    caster.call_method('Server_Skill_W',());assert len(active(caster))==1
casters[4].get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('Q',))
before=hp(targets[0]);result={'actual_w_cast':True,'unlearned_and_duplicate_rejected':True}
state={'phase':0,'start':u.GameplayStatics.get_time_seconds(world),'hit':False,'q_hit':False}
seen_impacts=set()
u.GameplayStatics.set_global_time_dilation(world,.4)
def capture(name):
    u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/name).as_posix()+'"',pc)
def finish(error=None):
    result['success']=error is None
    if error:result['error']=traceback.format_exc()
    u.GameplayStatics.set_global_time_dilation(world,1)
    u.unregister_slate_post_tick_callback(handle)
    (out/'result.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print('FIZZ_W_RESULT',result)
def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-state['start'];phase=state['phase']
        for component in impacts():
            seen_impacts.add(component.get_path_name())
            if abs(component.get_world_location().y)<500 and 'impact_particles' not in result:
                result['impact_particles']=component.get_num_active_particles()
                capture('FizzW_Splash.png')
        if phase==0 and age>=.5:
            comp=active(casters[0])[0]
            assert comp.get_num_active_particles()>0
            socket=str(comp.get_attach_socket_name());assert socket=='BUFFBONE_CSTM_WEAPON_1'
            assert (comp.get_world_location()-casters[0].mesh.get_socket_location(socket)).length()<.5
            result['weapon_bone_follow']=True
            capture('FizzW_Ready.png')
            casters[3].set_actor_hidden_in_game(True)
            casters[2].get_component_by_class(u.LOL_StateComponent).add_status_tag(tag('State.Dead'))
            state['phase']=1
        elif phase==1 and age>=.8:
            assert not active(casters[2]);assert not active(casters[3])[0].is_visible()
            casters[3].set_actor_hidden_in_game(False)
            result.update(death_cleanup=True,fog_hidden=True)
            casters[0].call_method('Server_ProcessMoveInput',(targets[0].get_actor_location(),targets[0],False))
            casters[4].call_method('Server_Skill_Q',(targets[4],))
            assert any(c.get_editor_property('template')==u.load_asset('/Game/VFX/Fizz/P_FizzQWater') for c in casters[4].get_components_by_class(u.ParticleSystemComponent)), 'Q cast rejected'
            state['phase']=2
        if state['phase']>=2 and not state['hit'] and not active(casters[0]):
            assert hp(targets[0])<before
            result['basic_attack_damage']=before-hp(targets[0]);result['basic_attack_consumed']=True
            capture('FizzW_Impact.png');state['hit']=True
        if state['phase']>=2 and age>=1.5 and not state['q_hit']:
            assert not active(casters[4]),'Q must consume W'
            assert active(casters[3])[0].is_visible(),'Fog visibility must recover'
            result.update(q_consumed=True,fog_restored=True);state['q_hit']=True
        if age>=5.8:
            assert state['hit'] and state['q_hit']
            assert len(seen_impacts)==2 and not impacts()
            assert not any(active(c) for c in casters)
            assert all(c.mesh.get_material(0)==m for c,m in zip(casters,originals))
            result.update(expiration_cleanup=True,body_material_preserved=True,one_shot_impacts=len(seen_impacts),impact_cleanup=True)
            finish()
    except Exception as error:finish(error)
handle=u.register_slate_post_tick_callback(tick)
