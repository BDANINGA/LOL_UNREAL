"""PIE integration: real R casting, arc/tumble, range, impact damage and knockback."""
import unreal as u
import json
from pathlib import Path
world = u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc = u.GameplayStatics.get_player_controller(world,0)
pc.set_view_target_with_blend(u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor)[0],0)
pawn = u.GameplayStatics.get_player_pawn(world,0)
if pawn:
    pawn.set_actor_hidden_in_game(True)
    pawn.set_actor_enable_collision(False)
casters = [u.GameplayStatics.get_all_actors_with_tag(world,name)[0] for name in ['FlightLong','FlightClose','FlightDiagonal']]
victim = u.GameplayStatics.get_all_actors_with_tag(world,'FlightVictim')[0]
victim.character_movement.set_editor_property('bRunPhysicsWithNoController',True)
victim.character_movement.set_movement_mode(u.MovementMode.MOVE_WALKING)
for actor, team in [(c,'Team.Blue') for c in casters]+[(victim,'Team.Red')]:
    tag = u.GameplayTag(); tag.import_text('(TagName="'+team+'")')
    actor.get_component_by_class(u.LOL_StateComponent).add_status_tag(tag)
    actor.set_actor_hidden_in_game(False)
for caster in casters:
    stat = caster.get_component_by_class(u.LOL_StatComponent)
    data = stat.get_editor_property('base_stat')
    data.set_editor_property('level',6)
    stat.set_editor_property('base_stat',data)
    caster.get_component_by_class(u.Champion_SkillComponent).call_method('Server_LevelUpSkill',('R',))
out = Path(u.Paths.project_saved_dir(),'DesignBackups'); out.mkdir(parents=True,exist_ok=True)
result = {'samples':[]}
state = {'shot':0,'impact':False,'close':False,'diagonal':False}
barrels, origins = [], []
floater_class = u.load_class(None,'/Game/UI/bp_damagefloater.bp_damagefloater_C')
victim_origin = victim.get_actor_location()
for caster, offset in zip(casters,[u.Vector(700,0,0),u.Vector(80,0,0),u.Vector(1200,900,0)]):
    origin = caster.get_actor_location()
    caster.call_method('Server_Skill_R',(origin+offset,))
    barrel = next((b for b in u.GameplayStatics.get_all_actors_of_class(world,u.GragasRBarrelVisual) if b.get_owner()==caster),None)
    assert barrel, 'Real R did not spawn flight actor'
    assert (barrel.get_actor_location()-(origin+u.Vector(0,0,40))).length()<.1, 'Spawn transform lost'
    barrels.append(barrel); origins.append(origin)
state['start'] = u.GameplayStatics.get_time_seconds(world)
state['last_x'] = origins[0].x
state['initial_rotation'] = tuple(getattr(barrels[0].get_component_by_class(u.StaticMeshComponent).get_editor_property('relative_rotation'),axis) for axis in ['pitch','yaw','roll'])
u.GameplayStatics.set_global_time_dilation(world,.3)

def capture(name):
    u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/name).as_posix()+'"',pc)

def finish(error=None):
    result['success'] = error is None
    if error: result['error'] = str(error)
    u.GameplayStatics.set_global_time_dilation(world,1)
    u.unregister_slate_post_tick_callback(handle)
    (out/'GragasRFlightTest.json').write_text(json.dumps(result,indent=2),encoding='utf8')
    print('GRAGAS_R_FLIGHT_RESULT',result)

def tick(dt):
    try:
        age = u.GameplayStatics.get_time_seconds(world)-state['start']
        if age < .97:
            assert u.SystemLibrary.is_valid(barrels[0]), 'Flight ended early'
            p = barrels[0].get_actor_location()
            assert p.x >= state['last_x']-.01
            state['last_x'] = p.x
            assert not u.GameplayStatics.get_all_actors_of_class(world,floater_class), 'Damage happened before impact'
        if state['shot']<3 and age >= [.2,.5,.85][state['shot']]:
            p = barrels[0].get_actor_location()
            assert origins[0].x < p.x < origins[0].x+700
            assert p.z > origins[0].z+65, 'Missing upward arc'
            rotation = barrels[0].get_component_by_class(u.StaticMeshComponent).get_editor_property('relative_rotation')
            values = tuple(getattr(rotation,a) for a in ['pitch','yaw','roll'])
            assert max(abs(a-b) for a,b in zip(values,state['initial_rotation']))>5, 'Missing tumble'
            result['samples'].append({'age':age,'x':p.x,'z':p.z,'rotation':values})
            capture('GragasR_Flight_'+str(state['shot'])+'.png')
            state['shot']+=1
        if not state['close'] and age>.2:
            assert not u.SystemLibrary.is_valid(barrels[1]), 'Short throw missed arrival cleanup'
            state['close']=True; result['short_throw_cleanup']=True
        if not state['impact'] and age>=1.02:
            assert not u.SystemLibrary.is_valid(barrels[0]), 'Barrel remained after impact'
            floaters = u.GameplayStatics.get_all_actors_of_class(world,floater_class)
            assert floaters, 'R impact did not deal damage'
            assert victim.get_editor_property('bIsKnockedBack'), 'R impact did not apply knockback'
            component = floaters[-1].get_component_by_class(u.WidgetComponent)
            widget = component.get_user_widget_object()
            tree = u.find_object(widget,'WidgetTree') or u.find_object(widget,'WidgetTree_0')
            text = u.find_object(tree,'text_damage')
            result['impact_damage_text']=str(text.get_text())
            result['impact_knockback']=True
            capture('GragasR_Impact.png')
            state['impact']=True
        if not state['diagonal'] and age>=1.35:
            assert u.SystemLibrary.is_valid(barrels[2]), 'Long diagonal throw ended early'
            p = barrels[2].get_actor_location()
            # 1500 requested units are clamped to the existing 1000-unit range.
            distance = ((p.x-origins[2].x)**2+(p.y-origins[2].y)**2)**.5
            assert 900<distance<=1000.1
            state['diagonal']=True
        if age>=1.65:
            assert all(not u.SystemLibrary.is_valid(b) for b in barrels)
            assert victim.get_actor_location().x>victim_origin.x+50, 'Victim did not move away from explosion'
            assert result['samples'][1]['z']>result['samples'][0]['z'] and result['samples'][1]['z']>result['samples'][2]['z']
            result['rise_and_fall']=True
            result['range_and_cleanup']=True
            finish()
    except Exception as error:
        finish(error)

handle = u.register_slate_post_tick_callback(tick)
