"""Read-only smoke check for the Codex to Unreal connection."""
import json
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
print(json.dumps({
    "project": unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()),
    "world": world.get_path_name() if world else None,
    "actor_count": len(actors),
    "actor_sample": [actor.get_actor_label() for actor in actors[:10]],
}, ensure_ascii=False))
