"""GREEN ION V15 loopback-only archive and dashboard endpoint tests."""
from __future__ import annotations
import hashlib
from http.client import HTTPConnection
from http.server import ThreadingHTTPServer
from pathlib import Path
import json
import sys
import tempfile
import threading
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))
import ground_station as gs  # noqa: E402

class LocalServerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory()
        cls.old_folder = gs.FLIGHT_DIR
        gs.FLIGHT_DIR = Path(cls.temporary.name) / "flights"
        cls.server = ThreadingHTTPServer(("127.0.0.1", 0), gs.Handler)
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls.port = cls.server.server_address[1]

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.thread.join(timeout=3)
        gs.FLIGHT_DIR = cls.old_folder
        cls.temporary.cleanup()

    def request(self, method, path, body=None, headers=None):
        conn = HTTPConnection("127.0.0.1", self.port, timeout=5)
        try:
            conn.request(method, path, body=body, headers=headers or {})
            response = conn.getresponse()
            return response.status, response.read()
        finally:
            conn.close()

    def test_local_server_serves_current_blue_dashboard(self):
        status, body = self.request("GET", "/")
        self.assertEqual(status, 200)
        self.assertIn(b'data-tab="recovery"', body)
        self.assertIn(b'homeAuto', body)
        self.assertIn(b'data-tab="help"', body)

    def test_local_archive_roundtrip_and_checksum(self):
        content = b"RKT4" + bytes(range(32)) * 4
        sha = hashlib.sha256(content).hexdigest()
        url = "/api/local/save?name=V12D001.BIN"
        status, body = self.request("POST", url, body=content,
                                    headers={"Content-Type": "application/octet-stream",
                                             "X-Green-Ion-SHA256": sha})
        self.assertEqual(status, 200)
        self.assertEqual(json.loads(body)["sha256"], sha)
        status, body = self.request("GET", "/api/local/files")
        self.assertEqual(status, 200)
        files = json.loads(body)["files"]
        self.assertTrue(any(f["name"] == "V12D001.BIN" and f["sha256"] == sha for f in files))
        status, retrieved = self.request("GET", f"/api/local/file?sha256={sha}&kind=bin")
        self.assertEqual(status, 200)
        self.assertEqual(content, retrieved)
        status, body = self.request("POST", url, body=content, headers={"X-Green-Ion-SHA256": sha})
        self.assertEqual(status, 200)
        self.assertTrue(json.loads(body)["duplicate"])

    def test_refuses_bad_checksum_traversal_and_cross_site_origin(self):
        content = b"good flight bytes"
        digest = hashlib.sha256(content).hexdigest()
        status, _ = self.request("POST", "/api/local/save?name=V12D002.BIN",
                                 body=content, headers={"X-Green-Ion-SHA256": "0"*64})
        self.assertEqual(status, 422)
        status, _ = self.request("POST", "/api/local/save?name=..%2Fsecret.BIN",
                                 body=content, headers={"X-Green-Ion-SHA256": digest})
        self.assertEqual(status, 400)
        status, _ = self.request("POST", "/api/local/save?name=V12D002.BIN",
                                 body=content, headers={"X-Green-Ion-SHA256": digest,
                                                       "Origin": "https://attacker.example"})
        self.assertEqual(status, 403)
        status, _ = self.request("GET", "/api/local/file?sha256=../../bad&kind=bin")
        self.assertEqual(status, 400)

    def test_exposes_only_public_cloud_browser_config(self):
        status, body = self.request("GET", "/config.js")
        self.assertEqual(status, 200)
        self.assertIn(b"sb_publishable_", body)
        self.assertNotIn(b"service_role", body)

if __name__ == "__main__":
    unittest.main()
