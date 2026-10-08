"""Create an unsaved isolated preview scene, never add effects to a game level."""
import unreal as u
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
world = u.EditorLoadingAndSavingUtils.new_blank_map(False)
world.get_world_settings().set_editor_property('default_game_mode', u.GameModeBase)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
position = u.Vector(0, -340, 200)
actors.spawn_actor_from_class(u.CameraActor, position,
    u.MathLibrary.find_look_at_rotation(position, u.Vector(0, 0, 100)))
perf = u.get_default_object(u.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings'))
perf.set_editor_property('bThrottleCPUWhenNotForeground', False)
u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_begin_play()
print('HIT_PREVIEW_READY')
