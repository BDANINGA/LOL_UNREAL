"""Temporary unsaved test scene. Restore GameStartMap after stopping PIE."""
import unreal as u
editor=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not editor.is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
world=u.EditorLoadingAndSavingUtils.new_blank_map(False)
world.get_world_settings().set_editor_property('default_game_mode',u.GameModeBase)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
pos=u.Vector(0,-900,650)
actors.spawn_actor_from_class(u.CameraActor,pos,u.MathLibrary.find_look_at_rotation(pos,u.Vector(0,0,60)))
actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,800),u.Rotator(pitch=-60,yaw=-20))
floor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector())
floor.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Plane'))
floor.set_actor_scale3d(u.Vector(100,100,1))
for i,name in enumerate(['Complete','Hidden','Death','LostTarget','Unlearned']):
    for cls,label,x in [(u.Champion_Fizz,name,-230),(u.Champion_Ezreal,name+'Target',200)]:
        actor=actors.spawn_actor_from_class(cls,u.Vector(x,i*1300,100))
        actor.set_actor_label(label);actor.set_editor_property('tags',[label])
        actor.set_editor_property('auto_possess_ai',u.AutoPossessAI.DISABLED)
editor.editor_request_begin_play()
