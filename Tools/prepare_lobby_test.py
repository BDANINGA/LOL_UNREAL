import unreal as u
from pathlib import Path
import re
import difflib

editor=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not editor.is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
audit=Path(u.Paths.project_dir(),'Saved/DesignBackups/LobbyBefore_20260909')
for name in ['wbp_lobby','wbp_lobby_slot']:
    bp=u.load_asset('/Game/Lobby/'+name)
    u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    task=u.AssetExportTask(); task.object=bp; task.exporter=u.ObjectExporterT3D()
    task.filename=str(audit/(name+'_after.t3d')); task.automated=True; task.prompt=False
    assert u.Exporter.run_asset_export_task(task)
    before=(audit/(name+'_before.t3d')).read_text(encoding='utf-16')
    after=(audit/(name+'_after.t3d')).read_text(encoding='utf-16')
    pattern=r'^   Begin Object Name="EventGraph".*?^   End Object'
    original=re.search(pattern,before,re.M|re.S).group(0)
    updated=re.search(pattern,after,re.M|re.S).group(0)
    # UE rebuilds the unused hidden ErrorTolerance pin on integer comparison nodes.
    def normalize(graph):
        graph=re.sub(r'PinFriendlyName=NSLOCTEXT\("", "[0-9A-F]+", "Completed"\)', 'PinFriendlyName="Completed"',graph)
        return '\n'.join(re.sub(r'PinId=[0-9A-F]+','PinId=GENERATED',line)
            if 'PinName="ErrorTolerance"' in line and 'bHidden=True' in line and 'LinkedTo=' not in line
            else line for line in graph.splitlines())
    original=normalize(original); updated=normalize(updated)
    if original!=updated:
        print('GRAPH_DIFF', ''.join(difflib.unified_diff(original.splitlines(True),updated.splitlines(True)))[:5000])
    assert original==updated, name+' EventGraph changed'
    print('EVENT_GRAPH_UNCHANGED',name)
assert editor.load_level('/Game/Lobby/Lobby')
editor.editor_request_begin_play()
print('LOBBY_PIE_REQUESTED')
