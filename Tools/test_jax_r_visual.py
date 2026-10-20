"""Actual R: persistent markers, cooldown, following, hidden/dead cleanup, original stat restoration."""
import unreal as u
import json
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn:
    pawn.set_actor_hidden_in_game(True);pawn.set_actor_enable_collision(False)
casters=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in ['RComplete','RMove','RDeath','RUnlearned']]
def layers(actor):
    result=[]
    for c in actor.get_components_by_class(u.StaticMeshComponent):
        m=c.get_material(0)
        if isinstance(m,u.MaterialInstanceDynamic) and '/Game/VFX/Jax/M_JaxR' in m.parent.get_path_name():result.append(c)
    return result
def stats(actor):
    data=actor.get_component_by_class(u.LOL_StatComponent).get_editor_property('base_stat')
    return (float(data.get_editor_property('Armor')),float(data.get_editor_property('SpellBlock')))
originals=[stats(a) for a in casters]
mesh_materials=[(a.mesh.get_material(0),a.mesh.get_material(1)) for a in casters]
casters[3].call_method('Server_Skill_R',())
assert not layers(casters[3]) and stats(casters[3])==originals[3]
for caster in casters[:3]:
    caster.set_actor_hidden_in_game(False)
    stat=caster.get_component_by_class(u.LOL_StatComponent)
    data=stat.get_editor_property('base_stat');data.set_editor_property('level',6);stat.set_editor_property('base_stat',data)
    caster.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('R',))
    caster.call_method('Server_Skill_R',())
    assert len(layers(caster))==2, 'R did not create armor and crest'
    before=layers(caster)
    caster.call_method('Server_Skill_R',())
    assert layers(caster)==before, 'Rejected recast duplicated markers'
buffed=[stats(a) for a in casters[:3]]
assert all(b[0]>o[0] and b[1]>o[1] for b,o in zip(buffed,originals))
out=Path(u.Paths.project_saved_dir(),'Validation/JaxR');out.mkdir(parents=True,exist_ok=True)
result={'actual_cast':True,'unlearned_and_recast_rejected':True,'armor_and_resistance_buff':True}
state={'phase':0,'start':u.GameplayStatics.get_time_seconds(world)}
def capture(name):
    u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/name).as_posix()+'"',pc)
def finish(error=None):
    result['success']=error is None
    if error:result['error']=str(error)
    u.unregister_slate_post_tick_callback(handle)
    (out/'result.json').write_text(json.dumps(result,indent=2),encoding='utf8')
    print('JAX_R_TEST_RESULT',result)
def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-state['start']
        phase=state['phase']
        if phase==0 and age>.6:
            capture('Jax_R_Active.png')
            before=[c.get_world_location() for c in layers(casters[1])]
            offset=u.Vector(160,80,0)
            casters[1].set_actor_location(casters[1].get_actor_location()+offset,False,False)
            state['expected']=[v+offset for v in before]
            casters[1].set_actor_hidden_in_game(True)
            dead=u.GameplayTag();dead.import_text('(TagName="State.Dead")')
            casters[2].get_component_by_class(u.LOL_StateComponent).add_status_tag(dead)
            state['phase']=1
        elif phase==1 and age>1:
            assert not layers(casters[2]), 'Death retained R markers'
            assert all(not c.is_visible() for c in layers(casters[1]))
            assert all((c.get_world_location()-v).length()<.1 for c,v in zip(layers(casters[1]),state['expected']))
            casters[1].set_actor_hidden_in_game(False)
            result.update(death_cleanup=True,fog_hidden=True,movement_following=True)
            state['phase']=2
        elif phase==2 and age>3.5:
            assert len(layers(casters[0]))==2 and all(c.is_visible() for c in layers(casters[1]))
            assert all((a.mesh.get_material(0),a.mesh.get_material(1))==m for a,m in zip(casters,mesh_materials))
            assert stats(casters[0])==buffed[0]
            capture('Jax_R_Sustained.png')
            result.update(persistent_after_cast=True,original_character_materials_preserved=True)
            state['phase']=3
        elif phase==3 and age>7:
            assert len(layers(casters[0]))==2
            result['late_buff_visible']=True
            state['phase']=4
        elif phase==4 and age>8.5:
            assert not any(layers(a) for a in casters)
            assert all(max(abs(x-y) for x,y in zip(stats(a),o))<.01 for a,o in zip(casters,originals))
            assert all(not a.get_component_by_class(u.JaxUltimateVisualComponent).is_component_tick_enabled() for a in casters)
            result.update(expiration_cleanup=True,original_stats_restored=True)
            finish()
    except Exception as error:finish(error)
handle=u.register_slate_post_tick_callback(tick)
