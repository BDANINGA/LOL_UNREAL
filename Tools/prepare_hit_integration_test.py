"""Unsaved integration fixtures. Does not modify any game map or Blueprint."""
import unreal as u
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
world = u.EditorLoadingAndSavingUtils.new_blank_map(False)
world.get_world_settings().set_editor_property('default_game_mode', u.LOL_GameModeBase)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
camp = actors.spawn_actor_from_class(u.TargetPoint, u.Vector(800, 0, 100))
camp.set_editor_property('tags', ['camp_wolf'])
for name, cls, position in [
    ('HitChampion', u.Champion_Ezreal, u.Vector(0, 0, 100)),
    ('HitMinion', u.Minion_Melee, u.Vector(400, 0, 100)),
    ('HitBuilding', u.Building_Turret, u.Vector(1200, 0, 150)),
    ('HitAttacker', u.Champion_Ezreal, u.Vector(-400, 0, 100)),
]:
    actor = actors.spawn_actor_from_class(cls, position)
    actor.set_actor_label(name)
    actor.set_editor_property('tags', [name])
    actor.set_editor_property('auto_possess_ai', u.AutoPossessAI.DISABLED)
    actor.set_replicates(True)
    if isinstance(actor, u.Character):
        actor.character_movement.set_movement_mode(u.MovementMode.MOVE_NONE)
camera_position = u.Vector(0, -480, 340)
actors.spawn_actor_from_class(u.CameraActor, camera_position,
    u.MathLibrary.find_look_at_rotation(camera_position, u.Vector(0, 0, 90)))
actors.spawn_actor_from_class(u.DirectionalLight, u.Vector(0, 0, 500), u.Rotator(pitch=-50, yaw=-35, roll=0))
u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_begin_play()
print('HIT_INTEGRATION_FIXTURES_READY')
