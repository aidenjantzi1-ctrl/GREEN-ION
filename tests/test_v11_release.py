from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class ReleaseTests(unittest.TestCase):
 def test_candidate_layout(self):
  for p in ('Green_Ion_V11/Green_Ion_V11.ino','Green_Ion_V11/Dashboard.h','Green_Ion_V11/PHONE_UPDATE.md','web/manifest.webmanifest','web/service-worker.js','web/advanced.mjs'): self.assertTrue((ROOT/p).is_file(),p)
 def test_version_and_safety(self):
  s=(ROOT/'Green_Ion_V11/Green_Ion_V11.ino').read_text(encoding='utf-8');self.assertIn('FIRMWARE_VERSION = "11.0-rc1"',s);self.assertNotIn('/api/ignition',s.lower())
if __name__=='__main__': unittest.main()
