"""Unsaved fixture for W weapon, empowered basic hit and Q consumption."""
import unreal as u
editor=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not editor.is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
world=u.EditorLoadingAndSavingUtils.new_blank_map(False)
world.get_world_settings().set_editor_property('default_game_mode',u.GameModeBase)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
pos=u.Vector(0,-650,440)
cam=actors.spawn_actor_from_class(u.CameraActor,pos,u.MathLibrary.find_look_at_rotation(pos,u.Vector(0,0,80)))
cam.camera_component.set_field_of_view(55)
actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,800),u.Rotator(pitch=-60,yaw=-20))
floor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector())
floor.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Plane'))
floor.set_actor_scale3d(u.Vector(100,100,1))
for i,name in enumerate(['Attack','Expire','Death','Hidden','Q','Unlearned']):
    for cls,label,x in [(u.Champion_Fizz,name,-65),(u.Champion_Ezreal,name+'Target',65)]:
        actor=actors.spawn_actor_from_class(cls,u.Vector(x,i*1500,100))
        actor.set_actor_label(label);actor.set_editor_property('tags',[label])
        actor.set_editor_property('auto_possess_ai',u.AutoPossessAI.DISABLED)
editor.editor_request_begin_play()
