"""Normal-speed actual Flash: both endpoints, readable tail, cooldown and cleanup."""
import unreal as u
import json
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn:
    pawn.set_actor_hidden_in_game(True);pawn.set_actor_enable_collision(False)
caster=u.GameplayStatics.get_all_actors_with_tag(world,'SpellCaster')[0]
caster.set_actor_hidden_in_game(False)
for name in ['SpellTarget','SpellAlly']:
    u.GameplayStatics.get_all_actors_with_tag(world,name)[0].set_actor_hidden_in_game(True)
spell=caster.get_component_by_class(u.LOL_SummonerSpellComponent)
templates=[u.load_asset('/Game/VFX/SummonerSpells/'+name) for name in ['P_FlashDeparture','P_FlashArrival']]
def flashes():
    return [c for c in world.get_world_settings().get_components_by_class(u.ParticleSystemComponent) if u.SystemLibrary.is_valid(c) and c.get_editor_property('template') in templates]
def net(where):
    value=u.Vector_NetQuantize()
    for axis in ['x','y','z']:value.set_editor_property(axis,getattr(where,axis))
    return value
out=Path(u.Paths.project_saved_dir(),'Validation/FlashVisibility');out.mkdir(parents=True,exist_ok=True)
# HighResShot can stall a frame; keep it off for accurate normal-speed samples.
capture_frames=False
u.GameplayStatics.set_global_time_dilation(world,1.)
origin=caster.get_actor_location()
spell.call_method('Server_CastFlash',(net(origin),))
assert not flashes(), 'Zero-distance cast emitted Flash'
spell.call_method('Server_CastFlash',(net(origin+u.Vector(900,0,0)),))
arrival=caster.get_actor_location()
distance=((arrival.x-origin.x)**2+(arrival.y-origin.y)**2)**.5
assert 1<distance<=spell.get_editor_property('FlashRange')+.1
assert len(flashes())==2
for template,position in zip(templates,[origin,arrival]):
    assert (next(c for c in flashes() if c.get_editor_property('template')==template).get_world_location()-position).length()<.1
spell.call_method('Server_CastFlash',(net(origin),))
assert (caster.get_actor_location()-arrival).length()<.1 and len(flashes())==2
result={'actual_cast':True,'endpoints_and_range':True,'cooldown_and_zero_distance_rejected':True,'normal_speed':True,'samples':[]}
state={'phase':0,'start':u.GameplayStatics.get_time_seconds(world)}
def finish(error=None):
    result['success']=error is None
    if error:result['error']=str(error)
    u.unregister_slate_post_tick_callback(handle)
    (out/'result.json').write_text(json.dumps(result,indent=2),encoding='utf8')
    print('FLASH_VISIBILITY_RESULT',result)
def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-state['start']
        phase=state['phase']
        if phase<3 and age >= [.07,.23,.4][phase]:
            effects=flashes()
            assert len(effects)==2
            counts=[next(c for c in effects if c.get_editor_property('template')==t).get_num_active_particles() for t in templates]
            assert min(counts)>0, 'An endpoint vanished too early'
            result['samples'].append({'age':age,'particles':counts})
            if capture_frames:
                u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/('Flash_'+str(phase)+'.png')).as_posix()+'"',pc)
            state['phase']+=1
        elif phase==3 and age>1.1:
            assert not flashes(), 'Flash did not auto-destroy'
            spell.set_editor_property('bEnableSpellEffects',False)
            spell.call_method('Multicast_PlayFlashEffect',(net(origin),net(arrival)))
            assert not flashes()
            spell.set_editor_property('bEnableSpellEffects',True)
            caster.set_actor_hidden_in_game(True)
            spell.call_method('Multicast_PlayFlashEffect',(net(origin),net(arrival)))
            assert not flashes()
            caster.set_actor_hidden_in_game(False)
            result.update(cleanup=True,hidden_and_disabled_suppressed=True)
            finish()
    except Exception as error:finish(error)
handle=u.register_slate_post_tick_callback(tick)
