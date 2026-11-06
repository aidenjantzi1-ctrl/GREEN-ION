"""HTTPS-only, password-paired local Companion access for the Ground Station."""

from __future__ import annotations

import hmac
from html import escape
from http import HTTPStatus
import secrets
import threading
import time
from urllib.parse import parse_qs, urlparse


SESSION_SECONDS = 2 * 60 * 60
ATTEMPT_WINDOW_SECONDS = 5 * 60
MAX_FAILED_ATTEMPTS = 5


class CompanionSecurity:
    def __init__(self, password: str, clock=time.monotonic) -> None:
        if len(password) < 16:
            raise ValueError("GREEN_ION_COMPANION_PASSWORD must contain at least 16 characters")
        self.password = password
        self.clock = clock
        self.sessions: dict[str, float] = {}
        self.failed: dict[str, list[float]] = {}
        self.lock = threading.RLock()

    @staticmethod
    def _respond(handler, status: int, body: bytes, *, content_type: str = "text/plain; charset=utf-8",
                 cookie: str | None = None, location: str | None = None) -> None:
        handler.send_response(status)
        handler.send_header("Content-Type", content_type)
        handler.send_header("Cache-Control", "no-store")
        handler.send_header("X-Content-Type-Options", "nosniff")
        handler.send_header("Referrer-Policy", "no-referrer")
        handler.send_header("Content-Security-Policy", "default-src 'none'; style-src 'unsafe-inline'; form-action 'self'; frame-ancestors 'none'")
        if cookie:
            handler.send_header("Set-Cookie", cookie)
        if location:
            handler.send_header("Location", location)
        handler.send_header("Content-Length", str(len(body)))
        handler.end_headers()
        handler.wfile.write(body)

    @staticmethod
    def _same_origin(handler) -> bool:
        origin = handler.headers.get("Origin", "")
        host = handler.headers.get("Host", "")
        parsed = urlparse(origin)
        return parsed.scheme == "https" and parsed.netloc == host and bool(host)

    @staticmethod
    def _cookie_token(handler) -> str | None:
        for part in handler.headers.get("Cookie", "").split(";"):
            key, separator, value = part.strip().partition("=")
            if separator and key == "green_ion_companion":
                return value
        return None

    def _authenticated(self, handler) -> bool:
        token = self._cookie_token(handler)
        if not token:
            return False
        with self.lock:
            now = self.clock()
            self.sessions = {key: expires for key, expires in self.sessions.items() if expires > now}
            return token in self.sessions

    def _login_page(self, error: str = "") -> bytes:
        message = f"<p role='alert'>{escape(error)}</p>" if error else ""
        return ("<!doctype html><html lang='en'><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>GREEN ION Companion pairing</title><style>body{font:18px system-ui;background:#061c32;color:#eef7ff;"
                "max-width:30rem;margin:12vh auto;padding:1.5rem}input,button{box-sizing:border-box;width:100%;padding:.8rem;margin:.6rem 0;"
                "font:inherit}button{background:#267ac3;color:white;border:0}p{line-height:1.5}</style>"
                "<h1>GREEN ION Companion</h1><p>Enter the laptop Ground Station pairing password. This local session expires after two hours.</p>"
                f"{message}<form method='post' action='/companion/login'><label>Password<input type='password' name='password' required autocomplete='current-password'></label>"
                "<button type='submit'>Connect</button></form></html>").encode()

    def handle_get(self, handler) -> bool:
        path = urlparse(handler.path).path
        if path == "/companion/login":
            if self._authenticated(handler):
                self._respond(handler, HTTPStatus.SEE_OTHER, b"", location="/")
            else:
                self._respond(handler, HTTPStatus.OK, self._login_page(), content_type="text/html; charset=utf-8")
            return True
        if not self._authenticated(handler):
            self._respond(handler, HTTPStatus.UNAUTHORIZED, self._login_page("Pair with the laptop before accessing flight data."),
                          content_type="text/html; charset=utf-8")
            return True
        return False

    def handle_post(self, handler) -> bool:
        path = urlparse(handler.path).path
        if not self._same_origin(handler):
            self._respond(handler, HTTPStatus.FORBIDDEN, b"Same-origin HTTPS request required")
            return True
        if path == "/companion/login":
            self._login(handler)
            return True
        if not self._authenticated(handler):
            self._respond(handler, HTTPStatus.UNAUTHORIZED, b"Companion session expired; pair again")
            return True
        if path == "/companion/logout":
            token = self._cookie_token(handler)
            with self.lock:
                self.sessions.pop(token, None)
            self._respond(handler, HTTPStatus.SEE_OTHER, b"",
                          cookie="green_ion_companion=; Max-Age=0; Path=/; Secure; HttpOnly; SameSite=Strict",
                          location="/companion/login")
            return True
        return False

    def _login(self, handler) -> None:
        if handler.headers.get("Content-Type", "").split(";", 1)[0].lower() != "application/x-www-form-urlencoded":
            self._respond(handler, HTTPStatus.BAD_REQUEST, b"Form request required")
            return
        try:
            length = int(handler.headers.get("Content-Length", "0"))
        except ValueError:
            length = 0
        if length <= 0 or length > 512:
            self._respond(handler, HTTPStatus.BAD_REQUEST, b"Invalid login request length")
            return
        address = handler.client_address[0]
        with self.lock:
            now = self.clock()
            attempts = [stamp for stamp in self.failed.get(address, []) if now - stamp < ATTEMPT_WINDOW_SECONDS]
            self.failed[address] = attempts
            if len(attempts) >= MAX_FAILED_ATTEMPTS:
                self._respond(handler, HTTPStatus.TOO_MANY_REQUESTS, self._login_page("Too many attempts. Try again in five minutes."), content_type="text/html; charset=utf-8")
                return
        try:
            submitted = parse_qs(handler.rfile.read(length).decode("utf-8"), strict_parsing=True)["password"][0]
        except (UnicodeError, ValueError, KeyError, IndexError):
            self._respond(handler, HTTPStatus.BAD_REQUEST, b"Invalid login form")
            return
        if not hmac.compare_digest(submitted, self.password):
            with self.lock:
                self.failed.setdefault(address, []).append(self.clock())
            self._respond(handler, HTTPStatus.UNAUTHORIZED, self._login_page("Incorrect password."), content_type="text/html; charset=utf-8")
            return
        token = secrets.token_urlsafe(32)
        with self.lock:
            self.failed.pop(address, None)
            self.sessions[token] = self.clock() + SESSION_SECONDS
        self._respond(handler, HTTPStatus.SEE_OTHER, b"", location="/",
                      cookie=f"green_ion_companion={token}; Max-Age={SESSION_SECONDS}; Path=/; Secure; HttpOnly; SameSite=Strict")
