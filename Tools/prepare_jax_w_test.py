"""Unsaved fixture for W casts, attack/Q consumption, expiration and death."""
import unreal as u

editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not editor.is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
world = u.EditorLoadingAndSavingUtils.new_blank_map(False)
world.get_world_settings().set_editor_property('default_game_mode', u.GameModeBase)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
position = u.Vector(470, -650, 440)
camera = actors.spawn_actor_from_class(u.CameraActor, position,
    u.MathLibrary.find_look_at_rotation(position, u.Vector(0, 0, 140)))
camera.camera_component.set_field_of_view(45)
actors.spawn_actor_from_class(u.DirectionalLight, u.Vector(0, 0, 700), u.Rotator(pitch=-55, yaw=-30))
floor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector())
floor.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Plane'))
floor.set_actor_scale3d(u.Vector(160, 160, 1))
for index, name in enumerate(['EmpowerExpire', 'EmpowerAttack', 'EmpowerDeath', 'EmpowerQ', 'EmpowerUnlearned']):
    actor = actors.spawn_actor_from_class(u.Champion_Jax, u.Vector(0, index * 1600, 100))
    actor.set_actor_label(name)
    actor.set_editor_property('tags', [name])
    actor.set_editor_property('auto_possess_ai', u.AutoPossessAI.DISABLED)
for index, name in [(1, 'AttackVictim'), (3, 'QVictim')]:
    victim = actors.spawn_actor_from_class(u.Champion_Ezreal, u.Vector(140, index * 1600, 100))
    victim.set_editor_property('tags', [name])
    victim.set_editor_property('auto_possess_ai', u.AutoPossessAI.DISABLED)
perf = u.get_default_object(u.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings'))
perf.set_editor_property('bThrottleCPUWhenNotForeground', False)
editor.editor_request_begin_play()
