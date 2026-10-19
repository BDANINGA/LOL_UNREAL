"""Run via runpy.run_path in PIE. Real E casts, damage, range, visibility and cleanup."""
import unreal as u
import json
import re
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn=u.GameplayStatics.get_player_pawn(world,0)
if pawn:
    pawn.set_actor_hidden_in_game(True); pawn.set_actor_enable_collision(False)
casters=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in ['EComplete','EMove','EDeath','EUnlearned']]
victims=[u.GameplayStatics.get_all_actors_with_tag(world,n)[0] for n in ['EInside','EOutside','EDeadVictim']]
out=Path(u.Paths.project_saved_dir())/'Validation'/'JaxE';out.mkdir(parents=True,exist_ok=True)

def tag(name):
    value=u.GameplayTag(); value.import_text('(TagName="'+name+'")'); return value

def hp(actor):
    task=u.AssetExportTask(); task.object=actor.get_component_by_class(u.LOL_StatComponent)
    task.filename=str(out/'stat.t3d');task.automated=True;task.prompt=False;task.replace_identical=True
    task.exporter=u.ObjectExporterT3D();assert u.Exporter.run_asset_export_task(task)
    return float(re.search(r'CurrentHP=([\d.]+)',(out/'stat.t3d').read_text()).group(1))

def layers(actor):
    return [c for c in actor.get_components_by_class(u.StaticMeshComponent)
            if isinstance(c.get_material(0),u.MaterialInstanceDynamic)
            and '/Game/VFX/Jax/M_JaxE' in c.get_material(0).parent.get_path_name()]

def capture(name):
    u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/name).as_posix()+'"',pc)

for actor,team in [(a,'Team.Blue') for a in casters]+[(a,'Team.Red') for a in victims]:
    status=actor.get_component_by_class(u.LOL_StateComponent)
    for name in ['Team.Blue','Team.Red','State.Dead']: status.remove_status_tag(tag(name))
    status.add_status_tag(tag(team)); actor.set_actor_hidden_in_game(False)
    position=actor.get_actor_location()
    position.z=actor.capsule_component.get_scaled_capsule_half_height()+2
    actor.set_actor_location(position,False,False)
victims[1].set_actor_location(u.Vector(950,0,132),False,False)
before=[hp(a) for a in victims]
result={}
casters[3].call_method('Server_Skill_E',());assert not layers(casters[3])
for caster in casters[:3]:
    caster.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('E',))
    caster.call_method('Server_Skill_E',());assert len(layers(caster))==3
    caster.call_method('Server_Skill_E',());assert len(layers(caster))==3
result.update(actual_cast=True,unlearned_and_duplicate_rejected=True)
# Slow only the test world enough to capture the short release phase reliably.
u.GameplayStatics.set_global_time_dilation(world,.35)
state={'phase':0,'start':u.GameplayStatics.get_time_seconds(world)}

def finish(error=None):
    result['success']=error is None
    if error: result['error']=str(error)
    u.GameplayStatics.set_global_time_dilation(world,1)
    u.unregister_slate_post_tick_callback(handle)
    (out/'result.json').write_text(json.dumps(result,indent=2),encoding='utf8')
    print('JAX_E_TEST_RESULT',result)

def tick(dt):
    try:
        age=u.GameplayStatics.get_time_seconds(world)-state['start']
        phase=state['phase']
        if phase==0 and age>.35:
            ground=layers(casters[0])[0]
            assert abs(ground.get_world_scale().x*50*.94-300)<.01
            state['spin_age']=layers(casters[0])[1].get_material(0).get_scalar_parameter_value('Age')
            capture('Jax_E_Spin.png')
            casters[1].set_actor_location(casters[1].get_actor_location()+u.Vector(160,0,0),False,False)
            casters[2].get_component_by_class(u.LOL_StateComponent).add_status_tag(tag('State.Dead'))
            state['phase']=1
        elif phase==1 and age>.65:
            assert not layers(casters[2]),'Death left rotating visuals'
            ground=layers(casters[1])[0]
            assert abs(ground.get_world_location().x-casters[1].get_actor_location().x)<.1
            assert layers(casters[0])[1].get_material(0).get_scalar_parameter_value('Age')>state['spin_age']
            casters[1].set_actor_hidden_in_game(True)
            result.update(radius_matches_damage=True,follows_movement=True,rotation_advances=True,death_cleanup=True)
            state['phase']=2
        elif phase==2 and age>.9:
            assert all(not c.is_visible() for c in layers(casters[1]))
            casters[1].set_actor_hidden_in_game(False)
            state['phase']=3
        elif phase==3 and age>1.2:
            assert all(c.is_visible() for c in layers(casters[1]))
            assert hp(victims[0])>=before[0], 'Damage occurred before E release'
            result['fog_visibility']=True
            state['phase']=4
        elif phase==4 and age>2.06:
            assert len(layers(casters[0]))==1, 'Release wave missing'
            assert 'M_JaxERelease' in layers(casters[0])[0].get_material(0).parent.get_name()
            assert hp(victims[0])<before[0], 'Inside target not damaged'
            result['target_hp_before']=before
            result['target_hp_after']=[hp(a) for a in victims]
            assert hp(victims[1])>=before[1], 'Outside target damaged'
            assert hp(victims[2])>=before[2], 'Dead caster triggered a delayed strike'
            capture('Jax_E_Release.png')
            result.update(release_wave=True,damage_on_release=True,outside_untouched=True,no_strike_after_death=True)
            state['release_location']=layers(casters[0])[0].get_world_location()
            casters[0].set_actor_location(casters[0].get_actor_location()+u.Vector(100,0,0),False,False)
            state['phase']=5
        elif phase==5 and age>2.20:
            assert (layers(casters[0])[0].get_world_location()-state['release_location']).length()<.1
            result['release_stays_at_impact']=True
            state['phase']=6
        elif phase==6 and age>2.6:
            assert not any(layers(a) for a in casters)
            assert all(not a.get_component_by_class(u.JaxCounterStrikeVisualComponent).is_component_tick_enabled() for a in casters)
            casters[0].call_method('Server_Skill_E',());assert not layers(casters[0])
            result.update(cleanup_and_idle_tick=True,cooldown_rejected=True)
            finish()
    except Exception as error:
        finish(error)

handle=u.register_slate_post_tick_callback(tick)
