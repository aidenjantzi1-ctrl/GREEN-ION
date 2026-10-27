from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
INO=(ROOT/'Green_Ion_V11/Green_Ion_V11.ino').read_text(encoding='utf-8')
DASH=(ROOT/'Green_Ion_V11/Dashboard.h').read_text(encoding='utf-8')
ADV=(ROOT/'web/advanced.mjs').read_text(encoding='utf-8')
DB=(ROOT/'web/history-db.mjs').read_text(encoding='utf-8')
README=(ROOT/'README.md').read_text(encoding='utf-8')

class OutstandingTests(unittest.TestCase):
 def test_minimal_feather_keeps_raw_and_moves_exports(self):
  self.assertNotIn('void writePostflightReport()',INO)
  self.assertNotIn('estimated_acceleration_mps2,estimated_vertical_apparent_g',INO)
  self.assertIn('Download the BIN and export CSV in the V11 browser analyzer',INO)
  for token in ('logSample(','saveCheckpoint();','recoverInterruptedFlights();','Update.begin','/api/download'):
   self.assertIn(token,INO)
  self.assertIn('decodeRkt4',DASH);self.assertIn('downloadRowsCsv',DASH)
 def test_replay_controls_timeline_and_graph(self):
  for token in ('drawReplay','analysis.samples','ctx.clearRect','last.timeS','last.altitudeM'):
   self.assertIn(token,ADV)
 def test_indexeddb_preserves_and_deduplicates_originals(self):
  for token in ('indexedDB.open','crypto.subtle.digest','originalBin','checksum','duplicate','exportHistory','restoreHistory'):
   self.assertIn(token,DB)
  self.assertIn('legacySummaries',DB)
 def test_release_identity(self):
  self.assertIn('# GREEN ION V11 Release Candidate',README)
  self.assertNotIn('# GREEN ION V10\n',README)
  self.assertIn('#define GREEN_ION_GIT_SHA "v11-standalone"',INO)

if __name__=='__main__':unittest.main()
