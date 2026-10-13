"""Import only W; reuse the projectile material/import pipeline without rebuilding Q/AA."""
import runpy
from pathlib import Path
import unreal as u
runpy.run_path(str(Path(u.Paths.project_dir()).resolve().parent/'Tools/import_ezreal_missile_meshes.py'),
              init_globals={'MISSILE_IMPORTS':[('SM_EzrealEssenceFlux','W')]})
