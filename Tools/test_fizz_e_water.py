"""Checks E stage timing and unchanged gameplay via real skill RPCs in PIE."""
import unreal as u
import json,re,traceback
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn:pawn.set_actor_hidden_in_game(True);pawn.set_actor_enable_collision(False)
names=['Natural','Fast','Hidden','Death','Unlearned','Destroy']
casters=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in names]
targets=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in ['Inside','Outside']]
out=Path(u.Paths.project_saved_dir())/'Validation/FizzE';out.mkdir(parents=True,exist_ok=True)
def tag(name):
    t=u.GameplayTag();t.import_text('(TagName="'+name+'")');return t
for actor,team in [(a,'Team.Blue') for a in casters]+[(a,'Team.Red') for a in targets]:
    status=actor.get_component_by_class(u.LOL_StateComponent)
    for n in ['State.Dead','Team.Blue','Team.Red']:status.remove_status_tag(tag(n))
    status.add_status_tag(tag(team));actor.set_actor_hidden_in_game(False)
    actor.character_movement.set_movement_mode(u.MovementMode.MOVE_NONE)
    pos=actor.get_actor_location();pos.z=actor.capsule_component.get_scaled_capsule_half_height()+2
    actor.set_actor_location(pos,False,False)
takeoff=u.load_asset('/Game/VFX/Fizz/P_FizzETakeoff');landing=u.load_asset('/Game/VFX/Fizz/P_FizzELanding')
def particles(template,row=None):
    return [c for c in world.get_world_settings().get_components_by_class(u.ParticleSystemComponent)
        if u.SystemLibrary.is_valid(c) and c.get_editor_property('template')==template
        and (row is None or abs(c.get_world_location().y-row*1600)<400)]
def rings(actor):
    return [c for c in actor.get_components_by_class(u.StaticMeshComponent)
        if isinstance(c.get_material(0),u.MaterialInstanceDynamic) and '/Game/VFX/Fizz/M_FizzERipple' in c.get_material(0).parent.get_path_name()]
def hp(actor):
    task=u.AssetExportTask();task.object=actor.get_component_by_class(u.LOL_StatComponent)
    task.filename=str(out/'stat.t3d');task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=u.ObjectExporterT3D()
    assert u.Exporter.run_asset_export_task(task)
    raw=(out/'stat.t3d').read_bytes();s=raw.decode('utf-16') if raw.startswith(b'\xff\xfe') else raw.decode('utf-8-sig')
    return float(re.search(r'CurrentHP=([\d.]+)',s).group(1))
before=[hp(a) for a in targets]
destinations=[a.get_actor_location()+u.Vector(360,0,0) for a in casters]
casters[4].call_method('Server_Skill_E',(destinations[4],));assert not rings(casters[4])
for i in [0,1,2,3,5]:
    casters[i].get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('E',))
    casters[i].call_method('Server_Skill_E',(destinations[i],))
    assert len(rings(casters[i]))==1 and len(particles(takeoff,i))==1
result={'actual_first_cast':True,'unlearned_rejected':True}
state={'phase':0,'start':u.GameplayStatics.get_time_seconds(world),'captured_landing':False}
seen_landing={};u.GameplayStatics.set_global_time_dilation(world,.25)
def capture(name):
    u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/name).as_posix()+'"',pc)
def finish(error=None):
    result['success']=error is None
    if error:result['error']=traceback.format_exc()
    u.GameplayStatics.set_global_time_dilation(world,1)
    u.unregister_slate_post_tick_callback(handle)
    (out/'result.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print('FIZZ_E_RESULT',result)
def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-state['start'];phase=state['phase']
        for i in [0,1]:
            if particles(landing,i) and i not in seen_landing:seen_landing[i]=age
        if phase==0 and age>=.12:
            assert particles(takeoff,0)[0].get_num_active_particles()>10
            assert not particles(landing)
            capture('FizzE_Takeoff.png')
            casters[1].call_method('Server_Skill_E',(destinations[1],))
            casters[1].call_method('Server_Skill_E',(destinations[1],))
            casters[2].set_actor_hidden_in_game(True)
            casters[3].get_component_by_class(u.LOL_StateComponent).add_status_tag(tag('State.Dead'))
            casters[5].destroy_actor();state['phase']=1
        elif phase==1 and age>=.25:
            assert not rings(casters[3]) and not particles(takeoff,3)
            assert not particles(takeoff,5)
            assert not rings(casters[2])[0].is_visible() and not particles(takeoff,2)[0].is_visible()
            assert abs(rings(casters[0])[0].get_world_location().x+180)<5
            casters[2].set_actor_hidden_in_game(False)
            result.update(death_cleanup=True,endplay_cleanup=True,fog_hidden=True,takeoff_anchored=True)
            state['phase']=2
        elif phase==2 and age>=.4:
            assert rings(casters[2])[0].is_visible();result['fog_restored']=True;state['phase']=3
        if 0 in seen_landing and age-seen_landing[0]>=.18 and not state['captured_landing']:
            plane=rings(casters[0])[0]
            assert abs(plane.get_world_location().x-180)<5
            assert abs(plane.get_world_scale().x*47-330)<.5
            assert plane.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION
            result['landing_particles']=particles(landing,0)[0].get_num_active_particles()
            assert result['landing_particles']>20
            capture('FizzE_Landing.png');state['captured_landing']=True
            result.update(landing_anchored=True,radius_330=True,cosmetic_no_collision=True)
        if age>=2.6:
            assert state['captured_landing'] and seen_landing[1]<seen_landing[0]-.15
            assert hp(targets[0])<before[0], 'Inside target should take E damage'
            assert hp(targets[1])==before[1], 'Far outside target should be untouched'
            assert not particles(takeoff) and not particles(landing)
            assert not any(rings(c) for c in casters[:5])
            result.update(natural_landing_time=seen_landing[0],recast_landing_time=seen_landing[1],damage=before[0]-hp(targets[0]),outside_range_unchanged=True,cleanup=True)
            finish()
    except Exception as error:finish(error)
handle=u.register_slate_post_tick_callback(tick)
