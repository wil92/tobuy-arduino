import runpy
from pathlib import Path

Import("env")

module = runpy.run_path(str(Path(env.subst("$PROJECT_DIR")) / "build_views.py"))
module["generate"]()
