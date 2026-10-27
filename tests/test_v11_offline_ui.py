from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
PAGE = (ROOT / "web/index.html").read_text(encoding="utf-8")
class OfflineUiTests(unittest.TestCase):
    def test_file_import_uses_local_analysis(self):
        for text in ('type="file"', 'arrayBuffer()', 'analysis-worker', 'localStorage', 'Download CSV', 'Offline'):
            self.assertIn(text, PAGE)
    def test_page_does_not_offer_recorder_controls(self):
        for forbidden in ('/api/arm', '/api/start', '/api/stop', '/api/firmware'):
            self.assertNotIn(forbidden, PAGE)
if __name__ == '__main__': unittest.main()
