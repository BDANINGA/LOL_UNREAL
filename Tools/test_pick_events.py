import unreal as u
import json
import re
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
pc=u.GameplayStatics.get_player_controller(world,0)
pick=u.WidgetLibrary.get_all_widgets_of_class(world,u.load_class(None,'/Game/ChampionSelectMap/wbp_pick.wbp_pick_C'),True)[0]
source=Path(u.Paths.project_dir(),'Saved/DesignBackups/PickBefore_20260909/pick_before.t3d').resolve().read_text(encoding='utf-16')
events=dict(re.findall(r'ComponentPropertyName="([^"]+)"(?:(?!End Object).)*?CustomFunctionName="([^"]+)"',source,re.S))
expected={'Alistar':'ALISTAR','Blitz':'BLITZCRANK','Ezreal':'EZREAL','Fizz':'FIZZ','Garen':'GAREN','Gragas':'GRAGAS','Jax':'JAX','LeeSin':'LEE_SIN','Tryndamere':'OLAF','Vayne':'VAYNE'}
results=[]
for key in expected:
    name='Button_'+key
    button=pick.get_editor_property(name)
    assert events[name] in str(button.get_editor_property('on_clicked')), name+' binding missing'
    button.get_editor_property('on_clicked').broadcast()
    selected=pc.player_state.get_editor_property('hovered_champion')
    results.append({'button':name,'selected':str(selected)})
    assert selected == getattr(u.ChampionID,expected[key]), key
assert len(set(r['selected'] for r in results))==10
print('BUTTON_EVENT_RESULTS',json.dumps(results))
pick.get_editor_property('Ready').get_editor_property('on_clicked').broadcast()
assert pc.player_state.get_editor_property('is_ready')
assert pc.player_state.get_editor_property('locked_champion') == u.ChampionID.VAYNE
print('LOCK_IN_PASSED')
u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
print('TEST_COMPLETE')
