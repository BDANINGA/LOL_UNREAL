"""Unsaved preview for actual Q and pooled basic-attack projectiles."""
import unreal as u
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
world=u.EditorLoadingAndSavingUtils.new_blank_map(False)
world.get_world_settings().set_editor_property('default_game_mode',u.GameModeBase)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
pos=u.Vector(0,-950,780)
actors.spawn_actor_from_class(u.CameraActor,pos,u.MathLibrary.find_look_at_rotation(pos,u.Vector(0,80,140)))
actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,700),u.Rotator(pitch=-60,yaw=-30,roll=0))
floor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,-30))
floor.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Plane'))
floor.set_actor_scale3d(u.Vector(40,40,1))
for cls,tag,pos in [(u.Champion_Ezreal,'MeshShooter',u.Vector(-650,240,100)),
                    (u.Minion_Melee,'MeshTarget',u.Vector(650,240,100))]:
    actor=actors.spawn_actor_from_class(cls,pos)
    actor.set_editor_property('tags',[tag])
    actor.set_editor_property('auto_possess_ai',u.AutoPossessAI.DISABLED)
    actor.character_movement.set_movement_mode(u.MovementMode.MOVE_NONE)
u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_begin_play()
