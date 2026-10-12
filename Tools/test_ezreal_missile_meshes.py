"""Verify mesh selection/orientation through actual Q and basic attack paths, then pooling."""
import unreal as u
import json
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn:
    pawn.set_actor_hidden_in_game(True);pawn.set_actor_enable_collision(False)
champ=u.GameplayStatics.get_all_actors_with_tag(world,'MeshShooter')[0]
target=u.GameplayStatics.get_all_actors_with_tag(world,'MeshTarget')[0]
for actor,team in [(champ,'Team.Blue'),(target,'Team.Red')]:
    tag=u.GameplayTag();tag.import_text('(TagName="'+team+'")')
    actor.get_component_by_class(u.LOL_StateComponent).add_status_tag(tag)
    actor.set_actor_hidden_in_game(True)
attack=champ.get_component_by_class(u.LOL_AttackComponent)
stat=champ.get_component_by_class(u.LOL_StatComponent)
data=stat.get_editor_property('base_stat')
data.set_editor_property('attack_range',2000.0)
stat.set_editor_property('base_stat',data)
u.GameplayStatics.set_global_time_dilation(world,0.4)
champ.call_method('Server_ProcessMoveInput',(target.get_actor_location(),target,False))
def active_bolts():
    return [a for a in u.GameplayStatics.get_all_actors_of_class(world,u.BaseProjectile) if a.is_actor_tick_enabled()]
result={}
out=Path(u.Paths.project_saved_dir(),'DesignBackups')
start=u.GameplayStatics.get_time_seconds(world)
state={'phase':0}
pool_count=len(u.GameplayStatics.get_all_actors_of_class(world,u.BaseProjectile))
def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-start
        assert age<10, 'Timed out waiting for actual basic attacks'
        if state['phase']==0 and active_bolts():
            champ.set_actor_tick_enabled(False)
            bolt=active_bolts()[0]
            mesh=bolt.get_component_by_class(u.StaticMeshComponent)
            assert mesh.static_mesh.get_name()=='SM_EzrealBasicBolt'
            assert mesh.get_world_scale().equals(u.Vector(1,1,1))
            champ.call_method('Multicast_SpawnEzrealProjectile',(0,u.Vector(-550,-130,170),u.Vector(650,-130,170),1.5,60.0,0.0,None,False))
            q=[a for a in u.GameplayStatics.get_all_actors_of_class(world,u.Actor) if a.get_component_by_class(u.EzrealQTrailComponent)][0]
            qmesh=q.get_component_by_class(u.StaticMeshComponent)
            assert qmesh.static_mesh.get_name()=='SM_EzrealMysticShot'
            assert qmesh.get_forward_vector().x>0.99
            result.update(q_mesh_selected=True,basic_mesh_selected=True,basic_scale=1.0)
            state.update(bolt=bolt,q=q,flight_start=age,phase=1)
        elif state['phase']==1 and age-state['flight_start']>=.5:
            bolt,q=state['bolt'],state['q']
            assert bolt.get_actor_location().x>-550 and q.get_actor_location().x>-500
            assert bolt.get_component_by_class(u.StaticMeshComponent).get_forward_vector().x>0.9
            trail=[a for a in u.GameplayStatics.get_all_actors_of_class(world,u.Emitter)
                   if a.get_component_by_class(u.ParticleSystemComponent).get_editor_property('template')==u.load_asset('/Game/Level/ezreal/FX/P_EzrealQTrail')]
            assert len(trail)==1 and trail[0].get_component_by_class(u.ParticleSystemComponent).get_num_active_particles()>0
            result.update(forward_orientation=True,flight=True,q_particles_preserved=True)
            filename=(out/'EzrealMissiles_Preview.png').as_posix()
            u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+filename+'"',pc)
            state['phase']=2
        elif state['phase']==2 and age-state['flight_start']>=1.8:
            assert not active_bolts(),'Basic projectile failed to deactivate on arrival'
            assert not u.SystemLibrary.is_valid(state['q']),'Q failed to expire'
            result['normal_cleanup']=True
            champ.set_actor_tick_enabled(True)
            state['phase']=3
        elif state['phase']==3 and active_bolts():
            champ.set_actor_tick_enabled(False)
            assert active_bolts()[0].get_component_by_class(u.StaticMeshComponent).static_mesh.get_name()=='SM_EzrealBasicBolt'
            assert len(u.GameplayStatics.get_all_actors_of_class(world,u.BaseProjectile))==pool_count
            result['pooled_reuse']=True
            state.update(phase=4,second_start=age)
        elif state['phase']==4 and age-state['second_start']>=1.6:
            assert not active_bolts()
            result['success']=True
            finish()
    except Exception as exc:
        result.update(success=False,error=str(exc));finish()
def finish():
    u.GameplayStatics.set_global_time_dilation(world,1)
    (out/'EzrealMissilesTest.json').write_text(json.dumps(result,indent=2),encoding='utf8')
    u.unregister_slate_post_tick_callback(handle)
    print('EZREAL_MISSILES_RESULT',result)
handle=u.register_slate_post_tick_callback(tick)
print('EZREAL_MISSILES_TEST_RUNNING')
