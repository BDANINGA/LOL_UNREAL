"""Run in a fresh prepare_ezreal_q_preview.py session to verify W flight and cleanup."""
import runpy
from pathlib import Path
import unreal as u

runpy.run_path(str(Path(u.Paths.project_dir()).resolve().parent / 'Tools/test_ezreal_q_trail.py'),
              init_globals={'TRAIL_KIND': 1})
