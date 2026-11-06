"""Optional local HTTPS/Chromium smoke. Requires openssl, Playwright and Chromium."""

from http.server import ThreadingHTTPServer
from pathlib import Path
import ssl
import subprocess
import sys
import tempfile
import threading

from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Ground_Station"))
from companion_security import CompanionSecurity, SESSION_SECONDS
from ground_station import Handler


with tempfile.TemporaryDirectory() as temporary:
    key = Path(temporary) / "key.pem"
    cert = Path(temporary) / "cert.pem"
    subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
                    "-keyout", str(key), "-out", str(cert), "-days", "1",
                    "-subj", "/CN=127.0.0.1", "-addext", "subjectAltName=IP:127.0.0.1"],
                   check=True, capture_output=True)
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    clock = {"now": 100.0}
    server.companion_security = CompanionSecurity("temporary-local-smoke-password",
                                                  clock=lambda: clock["now"])
    tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    tls.minimum_version = ssl.TLSVersion.TLSv1_2
    tls.load_cert_chain(str(cert), str(key))
    server.socket = tls.wrap_socket(server.socket, server_side=True)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base = f"https://127.0.0.1:{server.server_port}"
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(executable_path="/usr/bin/chromium", headless=True,
                                                 args=["--no-sandbox"])
            context = browser.new_context(ignore_https_errors=True, viewport={"width": 390, "height": 844})
            page = context.new_page()
            page.goto(base + "/")
            assert "Companion pairing" in page.title()
            unauthorized = context.request.get(base + "/api/flights")
            assert unauthorized.status == 401
            assert context.request.post(base + "/api/flight/nope/trash", data="{}",
                                        headers={"Origin": base}).status == 401
            page.fill("input[name=password]", "temporary-local-smoke-password")
            # The smoke certificate is deliberately untrusted, which gives Chromium
            # form submissions an opaque Origin. Pair via the API with an explicit
            # origin; a real phone must trust its certificate before pairing.
            paired = context.request.post(base + "/companion/login",
                                          form={"password": "temporary-local-smoke-password"},
                                          headers={"Origin": base}, max_redirects=0)
            assert paired.status == 303
            page.goto(base + "/")
            assert page.locator("#companionLogout").is_visible()
            page.wait_for_function("document.querySelector('#companionStatus').textContent.includes('PHONE CONNECTED')")
            assert context.request.get(base + "/api/flights").status == 200
            page.click("nav button[data-view=recovery]")
            page.fill("#manualLatitude", "37.1")
            page.fill("#manualLongitude", "-121.9")
            page.click("#setManualLocation")
            assert "manual" in page.locator("#userPositionStatus").inner_text().lower()
            page.click("nav button[data-view=analysis]")
            assert page.locator("#flightLibrary").is_visible()
            assert page.evaluate("document.documentElement.scrollWidth <= innerWidth")
            page.route("**/api/status", lambda route: route.abort())
            page.wait_for_function("document.querySelector('#companionStatus').textContent.includes('LAPTOP UNREACHABLE')")
            page.unroute("**/api/status")
            page.wait_for_function("document.querySelector('#companionStatus').textContent.includes('PHONE CONNECTED')")
            assert context.request.get(base + "/ground_station.py").status == 404
            assert context.request.post(base + "/api/flight/nope/trash", data="{}",
                                        headers={"Origin": "https://unrelated.example"}).status == 403
            page.click("#companionLogout")
            page.wait_for_url(base + "/companion/login")
            assert context.request.get(base + "/api/flights").status == 401
            paired = context.request.post(base + "/companion/login",
                                          form={"password": "temporary-local-smoke-password"},
                                          headers={"Origin": base}, max_redirects=0)
            assert paired.status == 303
            page.goto(base + "/")
            page.wait_for_function("document.querySelector('#companionStatus').textContent.includes('PHONE CONNECTED')")
            clock["now"] += SESSION_SECONDS + 1
            page.wait_for_url(base + "/companion/login", timeout=5000)
            browser.close()
    finally:
        server.shutdown()
        server.server_close()

print("Companion HTTPS browser smoke passed: pairing, protected API, cross-origin denial, outage/reconnect, logout, expiry, mobile width")
