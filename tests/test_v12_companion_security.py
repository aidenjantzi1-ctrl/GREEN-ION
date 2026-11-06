import io
from pathlib import Path
import subprocess
import sys
import unittest
from urllib.parse import urlencode

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))

from companion_security import CompanionSecurity, SESSION_SECONDS


class Request:
    def __init__(self, path="/api/flights", *, method="GET", password="", cookie="", origin="https://192.168.1.10:8765"):
        self.path = path
        self.client_address = ("192.168.1.20", 50000)
        self.headers = {"Host": "192.168.1.10:8765", "Origin": origin}
        body = urlencode({"password": password}).encode() if method == "POST" and path == "/companion/login" else b"{}"
        self.headers.update({"Content-Type": "application/x-www-form-urlencoded" if path == "/companion/login" else "application/json",
                             "Content-Length": str(len(body))})
        if cookie:
            self.headers["Cookie"] = cookie
        self.rfile = io.BytesIO(body)
        self.wfile = io.BytesIO()
        self.response_headers = {}

    def send_response(self, status):
        self.status = status

    def send_header(self, name, value):
        self.response_headers[name] = value

    def end_headers(self):
        pass


class CompanionSecurityTests(unittest.TestCase):
    def setUp(self):
        self.time = 100.0
        self.security = CompanionSecurity("long-local-pairing-secret", clock=lambda: self.time)

    def test_pairing_required_for_data_and_sensitive_posts(self):
        request = Request()
        self.assertTrue(self.security.handle_get(request))
        self.assertEqual(401, request.status)
        self.assertIn(b"Companion pairing", request.wfile.getvalue())
        request = Request("/api/flight/x/trash", method="POST")
        self.assertTrue(self.security.handle_post(request))
        self.assertEqual(401, request.status)

    def test_pairing_cookie_origin_and_expiration(self):
        login = Request("/companion/login", method="POST", password="long-local-pairing-secret")
        self.assertTrue(self.security.handle_post(login))
        self.assertEqual(303, login.status)
        cookie = login.response_headers["Set-Cookie"]
        self.assertIn("Secure; HttpOnly; SameSite=Strict", cookie)
        session_cookie = cookie.split(";", 1)[0]
        authorized = Request(cookie=session_cookie)
        self.assertFalse(self.security.handle_get(authorized))
        action = Request("/api/flight/x/trash", method="POST", cookie=session_cookie)
        self.assertFalse(self.security.handle_post(action))
        wrong_origin = Request("/api/flight/x/trash", method="POST", cookie=session_cookie,
                               origin="https://unrelated.example")
        self.assertTrue(self.security.handle_post(wrong_origin))
        self.assertEqual(403, wrong_origin.status)
        self.time += SESSION_SECONDS + 1
        expired = Request(cookie=session_cookie)
        self.assertTrue(self.security.handle_get(expired))
        self.assertEqual(401, expired.status)

    def test_failed_pairing_is_rate_limited(self):
        for _ in range(5):
            attempt = Request("/companion/login", method="POST", password="incorrect password")
            self.security.handle_post(attempt)
            self.assertEqual(401, attempt.status)
        locked = Request("/companion/login", method="POST", password="long-local-pairing-secret")
        self.security.handle_post(locked)
        self.assertEqual(429, locked.status)

    def test_server_rejects_wildcard_and_unprotected_lan_bind(self):
        script = ROOT / "Ground_Station" / "ground_station.py"
        for address, expected in (("0.0.0.0", "specific private LAN"),
                                  ("192.168.1.10", "requires --https-cert")):
            with self.subTest(address=address):
                result = subprocess.run([sys.executable, str(script), "--listen", address,
                                         "--no-browser"], capture_output=True, text=True, timeout=5)
                self.assertEqual(2, result.returncode)
                self.assertIn(expected, result.stderr)


if __name__ == "__main__":
    unittest.main()
