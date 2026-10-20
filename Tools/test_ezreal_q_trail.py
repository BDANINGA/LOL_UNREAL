"""Run after prepare_ezreal_q_preview.py. Uses real projectile multicast and world ticks."""
import unreal as u
from pathlib import Path
import json
kind = globals().get('TRAIL_KIND', 0)
skill = 'Q' if kind == 0 else 'W'
other_skill = 'W' if kind == 0 else 'Q'
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
camera=u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0]
pc.set_view_target_with_blend(camera,0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn: pawn.set_actor_hidden_in_game(True)
champ=u.GameplayStatics.get_all_actors_of_class(world,u.Champion_Ezreal)[0]
champ.set_editor_property('bEnable'+skill+'Trail',True)
champ.set_editor_property('bEnable'+other_skill+'Trail',False)
champ.set_actor_hidden_in_game(True)
champ.set_actor_enable_collision(False)
def emitters():
    # Q/W share this asset; R has its own effect.
    template=u.load_asset('/Game/Level/ezreal/FX/P_EzrealQTrail')
    return [a for a in u.GameplayStatics.get_all_actors_of_class(world,u.Emitter)
            if a.get_component_by_class(u.ParticleSystemComponent).get_editor_property('template')==template]
def fire(kind):
    champ.call_method('Multicast_SpawnEzrealProjectile',(kind,u.Vector(-550,0,120),u.Vector(650,0,120),1.5,60.0,0.0,None,False))
def missiles():
    return [a for a in u.GameplayStatics.get_all_actors_of_class(world,u.Actor) if a.get_component_by_class(u.EzrealQTrailComponent)]
fire(kind)
assert len(emitters())==1
assert len(missiles())==1
missile=missiles()[0]
if kind == 1:
    mesh = missile.get_component_by_class(u.StaticMeshComponent)
    assert mesh.static_mesh.get_name() == 'SM_EzrealEssenceFlux'
    assert mesh.get_forward_vector().x > 0.99
    assert missile.get_component_by_class(u.SphereComponent).get_unscaled_sphere_radius() == 60.0
start=u.GameplayStatics.get_time_seconds(world)
result={'spawned_'+skill.lower()+'_trail':True}; state={'phase':0}
output=Path(u.Paths.project_dir(),'Saved/DesignBackups/Ezreal'+skill+'TrailTest.json')
def tick(dt):
    try:
        elapsed=u.GameplayStatics.get_time_seconds(world)-start
        if state['phase']==0 and elapsed>=.6:
            e=emitters()[0]; particles=e.get_component_by_class(u.ParticleSystemComponent).get_num_active_particles()
            assert particles>0, 'No particles emitted'
            result['active_particles_in_flight']=particles
            result['missile_x_in_flight']=missile.get_actor_location().x
            assert result['missile_x_in_flight']>-500
            result['trail_scale']=e.get_actor_scale3d().x
            expected_scale=champ.get_editor_property(skill+'TrailScale')
            assert abs(result['trail_scale']-expected_scale)<0.001
            u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+str(Path(u.Paths.project_dir(),'Saved/DesignBackups/Ezreal'+skill+'Trail_Preview.png')).replace('\\','/')+'"',pc)
            state['phase']=1
        elif state['phase']==1 and elapsed>=.9:
            missile.destroy_actor()
            assert len(emitters())==1, 'Tail removed abruptly with projectile'
            result['tail_survives_projectile_destruction']=True
            state['phase']=2
        elif state['phase']==2 and elapsed>=1.85:
            assert len(emitters())==0,'Tail did not clean up'
            result['tail_cleanup']=True
            fire(1-kind); fire(2)
            assert len(emitters())==0,'Trail leaked to disabled skill/R'
            champ.set_editor_property('bEnable'+skill+'Trail',False)
            fire(kind)
            assert len(emitters())==0,'Trail toggle ignored'
            result['independent_toggles_and_r_exclusion']=True
            champ.set_editor_property('bEnable'+skill+'Trail',True)
            fire(kind)
            assert len(emitters())==1
            state['natural_start']=elapsed
            state['phase']=3
        elif state['phase']==3 and elapsed>=state['natural_start']+2.5:
            assert len(emitters())==0,'Trail leaked after normal projectile expiration'
            result['natural_expiration_cleanup']=True
            result['success']=True
            output.write_text(json.dumps(result,indent=2),encoding='utf8')
            u.unregister_slate_post_tick_callback(handle)
            print('EZREAL_'+skill+'_TRAIL_TEST_PASSED',result)
    except Exception as e:
        result['success']=False;result['error']=str(e)
        output.write_text(json.dumps(result,indent=2),encoding='utf8')
        u.unregister_slate_post_tick_callback(handle)
        u.log_error('EZREAL_'+skill+'_TRAIL_TEST_FAILED '+str(e))
handle=u.register_slate_post_tick_callback(tick)
print(skill+'_TRAIL_TEST_RUNNING')
