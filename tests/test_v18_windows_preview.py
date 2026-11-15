"""V18 preview launch regression: stable V17 stays default until release approval."""
import ast
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]

class V18PreviewLauncher(unittest.TestCase):
    def test_ground_station_preview_option_preserves_v17_default(self):
        code=(ROOT/"Ground_Station/ground_station.py").read_text(encoding="utf-8")
        ast.parse(code)
        self.assertIn('parser.add_argument("--v18-offline"',code)
        self.assertIn('if args.v18_offline and not (STATIC_ROOT / "v18-offline.html").is_file()',code)
        self.assertIn('"/v18-offline.html#flight" if args.v18_offline else ""',code)
        self.assertIn('webbrowser.open(browser_url)',code)
        self.assertIn('webbrowser.open, args=(browser_url,)',code)
        self.assertIn('NOT a qualified V18 release',code)
    def test_windows_preview_launcher_uses_existing_safe_entrypoint(self):
        command=(ROOT/"Ground_Station/START_V18_PREVIEW.cmd").read_text(encoding="utf-8")
        self.assertIn('START_GREEN_ION.cmd',command)
        self.assertIn('--v18-offline',command)
        self.assertNotIn('ota',command.lower())
        self.assertTrue((ROOT/"web/v18-offline.html").is_file())
        self.assertTrue((ROOT/"web/v18_service_worker.js").is_file())

if __name__=="__main__":
    unittest.main()
