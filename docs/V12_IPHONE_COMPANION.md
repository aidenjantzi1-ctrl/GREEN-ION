# iPhone Companion: local network setup and verification

The Windows laptop remains the Ground Station. The iPhone opens the laptop's six-page mobile layout over HTTPS; it does not connect to the rocket directly. The laptop polls Feather Wi-Fi and/or receives Uno USB LoRa telemetry. This software never controls ignition, staging or parachute deployment.

## Network arrangement

The default Ground Station binds only to `127.0.0.1` and is available only on the laptop. For Companion access, bind to one specific private IPv4 address on the laptop. Wildcard and public binds are refused. Use a trusted HTTPS certificate and a password of at least 16 characters.

| Arrangement | Expected links | Verification needed |
| --- | --- | --- |
| Local router plus two laptop Wi-Fi adapters | One adapter joins Feather AP for Wi-Fi telemetry; the other and iPhone join the router for Companion. Uno USB LoRa may also be connected. | Confirm Windows routing keeps both links live. |
| Local router plus Uno USB LoRa | Laptop and iPhone join router; laptop receives LoRa over USB. Feather AP need not be joined. | Ground LoRa hardware is currently disconnected, so this is NOT TESTED. |
| One laptop Wi-Fi adapter on Feather AP | Laptop receives Feather telemetry; phone normally cannot also reach the laptop through that adapter. | A second interface or a verified routing arrangement is needed for simultaneous phone access. |
| iPhone hotspot | Some Windows/iOS combinations may allow laptop access while phone uses cellular. | Local reachability, internet map tiles and cellular behavior are NOT TESTED; do not assume this works. |

Wi-Fi-only rocket operation remains supported without the phone. The Feather writes onboard SD independently of either laptop or phone connection.

## Windows setup

1. Put the laptop and iPhone on the intended local network. Find the laptop's private IPv4 address with `ipconfig`, such as `192.168.1.10`. Use an address actually assigned to the laptop. Keep the Windows firewall network profile **Private**. Allow inbound TCP port `8765` only from the trusted local network or phone address; do not forward the port from a router to the internet.
2. Create a certificate whose subject alternative name includes the laptop's selected IP. One option is `mkcert` on Windows. Install and trust its local CA on Windows, then generate the leaf certificate outside the repository, for example `mkcert -cert-file "$env:USERPROFILE\green-ion-phone.pem" -key-file "$env:USERPROFILE\green-ion-phone-key.pem" 192.168.1.10`. Replace the IP with the actual laptop address. `mkcert -CAROOT` identifies the local CA directory. Keep `rootCA-key.pem` and the server private key on the laptop; never copy private keys to the phone or GitHub.
3. Transfer only the local CA certificate (`rootCA.pem`) to the iPhone through a trusted method. Install its configuration profile and enable full trust for that root certificate in iOS Certificate Trust Settings. The address in Safari must match the certificate IP. If Safari warns that the certificate is untrusted, stop and fix trust before using Companion. A self-signed certificate bypass is not a supported field setup.
4. In PowerShell, set a private pairing password without putting it on the command line, then start the Ground Station from the repository root:

   ```powershell
   $env:GREEN_ION_COMPANION_PASSWORD = Read-Host "Companion pairing password (16+ characters)"
   py -3 Ground_Station\ground_station.py --listen 192.168.1.10 --http-port 8765 --https-cert "$env:USERPROFILE\green-ion-phone.pem" --https-key "$env:USERPROFILE\green-ion-phone-key.pem" --no-browser
   Remove-Item Env:GREEN_ION_COMPANION_PASSWORD
   ```

5. On the iPhone, open `https://192.168.1.10:8765/` in Safari and enter the pairing password. The browser gets a random, Secure, HttpOnly, SameSite=Strict session cookie that expires after two hours or when the Ground Station restarts. **Disconnect phone** signs out. The server limits repeated failed logins and rejects cross-origin POST requests. Every laptop data/control route uses this same authentication gate in Companion mode.

Only the main dashboard HTML and explicit API routes are served; arbitrary repository or data files cannot be fetched as static paths. The locally generated certificate and password are never committed. If the password is forgotten, stop the server, choose a new environment password and restart; existing sessions end.

## iPhone field verification checklist

These checks require the user's Windows laptop and iPhone and remain **NOT TESTED** in Codex Cloud.

1. Confirm the iPhone can load the HTTPS pairing page without a certificate warning. Before pairing, open an Analysis/API URL and confirm it asks for pairing instead of showing data.
2. Pair, open Mission Control, Recovery and Analysis at phone width, and check refresh, Wi-Fi/LoRa status, recorder state, graphs, map fallback, flight list, report and raw download. Keep the rocket stationary during setup.
3. In Recovery, permit Safari location. Confirm the phone position, accuracy/time, straight-line distance and true bearing. Deny location and confirm manual latitude/longitude still works while the rocket's last valid coordinate remains visible. Ordinary HTTP at a private IP is not a secure geolocation context.
4. Choose street map and optional Esri World Imagery satellite tiles, then disconnect external internet while retaining local access. Both tile sources require internet and may be unavailable or subject to provider terms. Confirm coordinates and the offline relative track stay available. Restore internet and test map tiles. Check whether iPhone cellular data continues to work alongside the chosen local connection; record the exact network arrangement and observed result.
5. Disconnect the rocket link without stopping its SD recorder. Confirm the laptop and phone show stale telemetry and preserve the last valid coordinate. Reconnect Feather Wi-Fi or LoRa and confirm an observed source return. Verify the actual BIN/SEN on SD afterward; browser behavior alone does not prove SD recording continuity.
6. Sign out and confirm the flight library and protected actions require pairing again. Restart the Ground Station and verify old sessions expire. Check the Windows firewall rule and that the service is unreachable from an untrusted network.

Cloud evidence: Python unit tests cover pairing, session expiration, rate limiting, cross-origin requests, and rejection of unprotected LAN/wildcard binds. A local HTTPS Chromium smoke with a throwaway certificate covers access gating, pairing, static-path denial, cross-origin action denial, logout and 390-pixel layout. Its certificate is deliberately untrusted; that smoke uses an explicit same-origin API login. Trusted iPhone Safari operation, geolocation and cellular routing still require the physical checks above.
