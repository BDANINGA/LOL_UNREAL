"""Temporary PIE scene only; never saves or changes the gameplay map."""
import unreal as u
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
assert all(p.get_path_name().startswith('/Temp/') for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages())
w = u.EditorLoadingAndSavingUtils.new_blank_map(False)
w.get_world_settings().set_editor_property('default_game_mode', u.LOL_GameModeBase)
e = u.get_editor_subsystem(u.EditorActorSubsystem)
for cls, location, tag in [(u.Champion_Vayne,u.Vector(0,0,140),'VayneETest'), (u.Champion_Ezreal,u.Vector(450,0,140),'CondemnTarget')]:
    actor = e.spawn_actor_from_class(cls,location)
    actor.tags = [tag]
    actor.set_editor_property('auto_possess_ai',u.AutoPossessAI.DISABLED)
floor = e.spawn_actor_from_class(u.StaticMeshActor,u.Vector(400,0,0))
floor.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Plane'))
floor.set_actor_scale3d(u.Vector(30,30,1))
wall = e.spawn_actor_from_class(u.StaticMeshActor,u.Vector(820,0,130))
wall.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'))
wall.set_actor_scale3d(u.Vector(.6,5,2.6))
wall.tags = ['CondemnWall']
e.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,900),u.Rotator(pitch=-55,yaw=-35))
camera_pos = u.Vector(420,-1000,1200)
camera = e.spawn_actor_from_class(u.CameraActor,camera_pos,u.MathLibrary.find_look_at_rotation(camera_pos,u.Vector(420,0,70)))
camera.camera_component.set_field_of_view(48)
camera.tags = ['CondemnCamera']
u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_begin_play()
print('VAYNE_E_TEST_SCENE_READY')
