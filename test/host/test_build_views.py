import importlib.util
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("build_views", ROOT / "build_views.py")
build_views = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build_views)


class BuildViewsTest(unittest.TestCase):
    def test_header_matches_source(self):
        source = build_views.SOURCE.read_text(encoding="utf-8")
        self.assertEqual(build_views.OUTPUT.read_text(encoding="utf-8"), build_views.render(source))
        subprocess.run(["python3", str(ROOT / "build_views.py"), "--check"], check=True)

    def test_raw_string_collision_is_rejected(self):
        with self.assertRaises(ValueError):
            build_views.render('example )TOBUY_PAGE" example')

    def test_source_is_embedded_verbatim(self):
        source = '<p>á & "quoted"</p>\n'
        self.assertIn(source, build_views.render(source))


if __name__ == "__main__":
    unittest.main()
