"""Run after prepare_vayne_e_test.py; inspect real E cast, wall hit and cleanup."""
import json
from pathlib import Path
import unreal as u
w = u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc = u.GameplayStatics.get_player_controller(w,0)
vayne = u.GameplayStatics.get_all_actors_with_tag(w,'VayneETest')[0]
target = u.GameplayStatics.get_all_actors_with_tag(w,'CondemnTarget')[0]
camera = u.GameplayStatics.get_all_actors_with_tag(w,'CondemnCamera')[0]
pc.possess(vayne)
pc.set_view_target_with_blend(camera,0)
perf = u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'))
old_throttle = perf.get_editor_property('bThrottleCPUWhenNotForeground')
perf.set_editor_property('bThrottleCPUWhenNotForeground',False)
for actor,team in [(vayne,'Team.Blue'),(target,'Team.Red')]:
    state = actor.get_component_by_class(u.LOL_StateComponent)
    for name in ['Team.Blue','Team.Red']:
        tag=u.GameplayTag();tag.import_text('(TagName="'+name+'")');state.remove_status_tag(tag)
    tag=u.GameplayTag();tag.import_text('(TagName="'+team+'")');state.add_status_tag(tag)
    actor.character_movement.set_movement_mode(u.MovementMode.MOVE_NONE)
skills = vayne.get_component_by_class(u.Champion_SkillComponent)
skills.call_method('Server_LevelUpSkill',('E',))
u.GameplayStatics.set_global_time_dilation(w,.12)
out=Path(u.Paths.project_saved_dir(),'Validation/VayneE');out.mkdir(parents=True,exist_ok=True)
state={'start':u.GameplayStatics.get_time_seconds(w),'seen':set(),'cast':False}
results={}
def capture(name):
    u.SystemLibrary.execute_console_command(w,'shot showui filename="'+(out/(name+'.png')).as_posix()+'" -nosuffix',pc)
def finish():
    u.unregister_slate_post_tick_callback(handle)
    u.GameplayStatics.set_global_time_dilation(w,1)
    perf.set_editor_property('bThrottleCPUWhenNotForeground',old_throttle)
    (out/'visual-result.json').write_text(json.dumps(results,indent=2),encoding='utf8')
    print('VAYNE_E_VISUAL_RESULT',results)
def tick(dt):
    try:
        elapsed=u.GameplayStatics.get_time_seconds(w)-state['start']
        if elapsed>.5 and not state['cast']:
            vayne.call_method('Server_ExecuteCondemn',(target,))
            state['cast']=True
        bolts=u.GameplayStatics.get_all_actors_of_class(w,u.VayneCondemnProjectile)
        for bolt in bolts:
            body=[m for m in bolt.get_components_by_class(u.StaticMeshComponent) if m.get_name()=='CondemnBolt'][0]
            phase=0 if body.is_visible() else (2 if bolt.get_actor_location().x>700 else 1)
            if phase==0 and bolt.get_actor_location().x>170 and 'flight' not in state['seen']:
                capture('01_E_Flight');state['seen'].add('flight')
                results['flight_seen']=True
            if phase==1 and 'hit' not in state['seen']:
                capture('02_E_Impact');state['seen'].add('hit');results['impact_seen']=True
            if phase==2 and 'wall' not in state['seen']:
                state['wall_time']=elapsed;state['seen'].add('wall');results['wall_seen']=True
        if 'wall' in state['seen'] and elapsed-state['wall_time']>.06 and 'wallshot' not in state['seen']:
            capture('03_E_Wall');state['seen'].add('wallshot')
            results['wall_stop']=str(target.get_actor_location())
        if elapsed>2.2:
            results['effects_cleaned']=len(bolts)==0
            results['passed']=all(results.get(key,False) for key in ['flight_seen','impact_seen','wall_seen','effects_cleaned'])
            finish()
    except Exception as exc:
        results['error']=str(exc);finish()
handle=u.register_slate_post_tick_callback(tick)
print('VAYNE_E_VISUAL_TEST_RUNNING')
