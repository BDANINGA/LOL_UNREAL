"""Exercise real R RPCs in the unsaved fixture, capture the fin and warning."""
import unreal as u
import json,re,traceback
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn:pawn.set_actor_hidden_in_game(True);pawn.set_actor_enable_collision(False)
casters=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in ['Miss','Hit','Destroy','Unlearned','Lost']]
victim=u.GameplayStatics.get_all_actors_with_tag(world,'Victim')[0]
lost=u.GameplayStatics.get_all_actors_with_tag(world,'LostTarget')[0]
out=Path(u.Paths.project_saved_dir())/'Validation/FizzR';out.mkdir(parents=True,exist_ok=True)
def tag(name):
    t=u.GameplayTag();t.import_text('(TagName="'+name+'")');return t
for actor,team in [(a,'Team.Blue') for a in casters]+[(victim,'Team.Red'),(lost,'Team.Red')]:
    status=actor.get_component_by_class(u.LOL_StateComponent)
    for n in ['State.Dead','Team.Blue','Team.Red']:status.remove_status_tag(tag(n))
    status.add_status_tag(tag(team));actor.set_actor_hidden_in_game(False)
    actor.character_movement.set_movement_mode(u.MovementMode.MOVE_NONE)
    pos=actor.get_actor_location();pos.z=actor.capsule_component.get_scaled_capsule_half_height()+2
    actor.set_actor_location(pos,False,False)
def parts(actor,word):
    return [c for c in actor.get_components_by_class(u.StaticMeshComponent) if word in c.get_name()]
def warning(actor):
    comps=parts(actor,'FizzRSurface')
    return comps and 'M_FizzRWarning' in comps[0].get_material(0).parent.get_path_name()
def hp(actor):
    task=u.AssetExportTask();task.object=actor.get_component_by_class(u.LOL_StatComponent)
    task.filename=str(out/'stat.t3d');task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=u.ObjectExporterT3D()
    assert u.Exporter.run_asset_export_task(task)
    raw=(out/'stat.t3d').read_bytes();s=raw.decode('utf-16') if raw.startswith(b'\xff\xfe') else raw.decode('utf-8-sig')
    return float(re.search(r'CurrentHP=([\d.]+)',s).group(1))
dest=[u.Vector(600,i*2000,100) for i in range(5)]
casters[3].call_method('Server_Skill_R',(dest[3],));assert not parts(casters[3],'FizzR')
u.SystemLibrary.execute_console_command(world,'summon /Script/LOL_UNREAL.LOL_PlayerController',pc)
debug_pc=u.GameplayStatics.get_all_actors_of_class(world,u.LOL_PlayerController)[-1]
for actor in casters[:3]+[casters[4]]:
    debug_pc.possess(actor);debug_pc.call_method('Server_SetDebugMode',(True,))
    for _ in range(5):debug_pc.call_method('Server_DebugLevelUp',())
    debug_pc.call_method('Server_SetDebugMode',(False,))
    actor.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('R',))
debug_pc.un_possess();debug_pc.set_actor_tick_enabled(False)
before=hp(victim)
for actor,to in zip(casters[:3]+[casters[4]],dest[:3]+[dest[4]]):
    actor.call_method('Server_Skill_R',(to,));assert len(parts(actor,'FizzRFin'))==1
    actor.call_method('Server_Skill_R',(to,));assert len(parts(actor,'FizzRFin'))==1
result={'actual_cast':True,'unlearned_rejected':True,'duplicate_rejected':True}
state={'start':u.GameplayStatics.get_time_seconds(world),'phase':0,'hit_seen':False,'moved':False,'restored':False,'warn_shot':False,'erupt_shot':False}
u.GameplayStatics.set_global_time_dilation(world,.3)
def capture(name):u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/name).as_posix()+'"',pc)
def finish(error=None):
    result['success']=error is None
    if error:result['error']=traceback.format_exc()
    u.GameplayStatics.set_global_time_dilation(world,1);u.unregister_slate_post_tick_callback(handle)
    (out/'result.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print('FIZZ_R_RESULT',result)
def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-state['start']
        if state['phase']==0 and age>.65:
            fin=parts(casters[0],'FizzRFin')[0];surface=parts(casters[0],'FizzRSurface')[0]
            assert -130<fin.get_world_location().x<100 and fin.get_world_location().z<20
            assert fin.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION
            assert not warning(casters[0]);capture('FizzR_Travel.png')
            casters[2].destroy_actor();result.update(grounded_travel=True,cosmetic_no_collision=True);state['phase']=1
        if warning(casters[4]) and not result.get('destroyed_target'):
            lost.destroy_actor();result['destroyed_target']=True;state['lost_time']=age
        if result.get('destroyed_target') and not result.get('target_destroy_fallback') and age>state['lost_time']+.15:
            center=parts(casters[4],'FizzRSurface')[0].get_world_location()
            assert abs(center.x-600)<1 and abs(center.y-8000)<1
            result['target_destroy_fallback']=True
        if warning(casters[1]) and not state['moved']:
            state['hit_seen']=True
            victim.set_actor_location(u.Vector(750,2100,victim.get_actor_location().z),False,False)
            victim.set_actor_hidden_in_game(True);state['move_time']=age;state['moved']=True
        if state['moved'] and not state['restored'] and age>state['move_time']+.15:
            surface=parts(casters[1],'FizzRSurface')[0]
            assert (surface.get_world_location()-u.Vector(750,2100,5)).length()<10
            assert not surface.is_visible()
            victim.set_actor_hidden_in_game(False);state['restored']=True
            result.update(target_follow=True,fog_hidden=True)
        if state['restored'] and warning(casters[1]) and age>state['move_time']+.3:
            assert parts(casters[1],'FizzRSurface')[0].is_visible();result['fog_restored']=True
        if warning(casters[0]) and not state['warn_shot'] and age>2.65:
            surface=parts(casters[0],'FizzRSurface')[0]
            assert abs(surface.get_world_scale().x*47-300)<.5
            assert abs(surface.get_world_location().x-600)<1
            assert surface.get_material(0).get_scalar_parameter_value('Progress')>.3
            capture('FizzR_Warning.png');state['warn_shot']=True
            result.update(radius_300=True,miss_endpoint=True,countdown=True)
        if state['warn_shot'] and not parts(casters[0],'FizzRSurface') and not state['erupt_shot']:
            sharks=[c for a in u.GameplayStatics.get_all_actors_of_class(world,u.Actor) for c in a.get_components_by_class(u.SkeletalMeshComponent) if c.get_name()=='FizzSharkMesh' and abs(c.get_world_location().y)<500]
            assert sharks
            state['eruption_time']=age;state['erupt_shot']=True
        if state['erupt_shot'] and not result.get('eruption') and age>state['eruption_time']+.25:
            capture('FizzR_Eruption.png');result['eruption']=True
        if age>5.5:
            assert state['hit_seen'] and state['restored'] and result.get('eruption') and result.get('target_destroy_fallback') and result.get('fog_restored')
            assert hp(victim)<before
            assert not any(parts(a,'FizzR') for a in [casters[0],casters[1],casters[3],casters[4]])
            effects=[c for c in world.get_world_settings().get_components_by_class(u.ParticleSystemComponent) if u.SystemLibrary.is_valid(c) and 'Fizz' in c.get_editor_property('template').get_name()]
            assert not effects
            result.update(damage=before-hp(victim),cleanup=True,endplay_cleanup=True)
            finish()
    except Exception as error:finish(error)
handle=u.register_slate_post_tick_callback(tick)
