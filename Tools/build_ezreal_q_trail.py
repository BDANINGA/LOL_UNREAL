"""Run in a compiled editor after create_ezreal_q_material.py to author the Q-only trail."""
import unreal as u
from pathlib import Path
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world,'LOL.Ezreal.BuildQTrail')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/Level/ezreal/FX'],True)
ps=u.load_asset('/Game/Level/ezreal/FX/P_EzrealQTrail')
assert ps
task=u.AssetExportTask();task.object=ps;task.exporter=u.ObjectExporterT3D()
task.filename=str(Path(u.Paths.project_dir(),'Saved/DesignBackups/EzrealQTrail.t3d'));task.automated=True;task.prompt=False
assert u.Exporter.run_asset_export_task(task)
print('EZREAL_Q_TRAIL_ASSET_READY')
