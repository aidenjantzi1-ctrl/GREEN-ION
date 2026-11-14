# GREEN ION V16.3 — Home Wi-Fi + Direct Wi-Fi

GREEN ION can keep its own direct Wi-Fi access point active while also joining a saved **2.4 GHz** home Wi-Fi network. The primary flight recorder remains local to the Feather and microSD; home Wi-Fi is optional and is never required for recording.

## Network modes

- **AP only:** GREEN ION broadcasts `GREEN-ION-V16-3` and serves the dashboard at `http://192.168.4.1`. This is used when no home network is saved, or when home Wi-Fi is manually disconnected/forgotten.
- **AP+STA:** the direct GREEN ION network stays active while the Feather also joins the saved home router. The router assigns the home-network IP by DHCP.
- If the router becomes unavailable, the direct access point remains available. GREEN ION retries the saved home network on a bounded schedule only when the recorder is not in a critical flight state.

The ESP32 radio used by the Feather ESP32 V2 is a **2.4 GHz Wi-Fi radio**. A 5 GHz-only SSID cannot be used. A dual-band router is fine when it provides a compatible 2.4 GHz network.

## Security model

Home Wi-Fi passwords and an optional custom direct-AP password are stored locally in ESP32 Preferences/NVS. They are never hardcoded into the repository, returned by the Wi-Fi status API, exported with flight data, or intentionally printed to Serial.

Changing Wi-Fi credentials requires a separate local management authorization:

1. Open the GREEN ION dashboard while physically next to the Feather.
2. Press the Feather's **SW38** user button once.
3. Within 60 seconds, choose **AUTHORIZE WI-FI CHANGES** in Settings & Advanced → Wi-Fi settings.
4. The Feather issues a temporary RAM-only management session for that browser. Requests also require a per-browser CSRF value.
5. Choose **FINISH WI-FI SETUP** when done. GREEN ION restarts stationary calibration before automatic launch watching resumes.

The normal direct-AP password by itself does **not** authorize credential changes.

## Flight-safety behavior

Network reconfiguration, Wi-Fi scanning and credential writes are blocked while GREEN ION is:

- watching for launch;
- recording a flight;
- running Full Home Test;
- in firmware maintenance / OTA; or
- already performing another Wi-Fi setup operation.

Starting Wi-Fi management physically pauses automatic launch watching and requires a fresh stationary calibration when setup finishes. Home-Wi-Fi reconnect work never runs in the 50 Hz sampling/recording critical state. The microSD recorder does not depend on AP, STA, internet, Vercel, Supabase or the phone remaining connected.

## Connect from an iPhone

1. Install the AP+STA firmware build on the Feather on a safe bench with all rocket motors removed.
2. On iPhone open **Settings → Wi-Fi** and join **GREEN-ION-V16-3** using the current direct Wi-Fi password.
3. Open Safari and go to **http://192.168.4.1**.
4. Open **Settings & Advanced → Wi-Fi settings**.
5. Press **SW38** on the Feather, then tap **AUTHORIZE WI-FI CHANGES** within 60 seconds.
6. Tap **SCAN 2.4 GHz NETWORKS**. Select your home network, or type the SSID manually for a hidden network.
7. Enter the home Wi-Fi password and tap **CONNECT**. The password field is cleared after submission.
8. When connected, GREEN ION shows both addresses:
   - Direct: `http://192.168.4.1`
   - Home: the DHCP address assigned by your router, such as `http://192.168.1.xxx`
9. Copy the Home address if desired. Rejoin your normal home Wi-Fi on the iPhone and open that copied address in Safari to reach GREEN ION through the router.
10. Tap **FINISH WI-FI SETUP** and keep the Feather stationary while calibration completes.

## Return to direct Wi-Fi

Direct access remains available at `http://192.168.4.1` whenever the Feather is powered.

To temporarily use AP-only mode while retaining the saved home network, authorize Wi-Fi changes and select **DISCONNECT HOME WI-FI**. A reboot will try the saved home network again.

To remain AP-only across reboots, authorize Wi-Fi changes and select **FORGET SAVED NETWORK**. This removes the saved home SSID/password from NVS; it does not erase flight recordings or other settings.

## Change the direct access-point password

In the authorized Wi-Fi Settings session, enter a new 8–63 character password under **Direct GREEN ION Wi-Fi password** and save it. The new password is stored but deliberately takes effect only after the next Feather reboot, preventing an unexpected disconnect during setup.

## Validation status

Software tests cover AP-only policy, successful/failed connection state, bounded router-unavailable retries, disconnect/reconnect policy, forgetting credentials, both dashboard addresses, authorization/CSRF requirements, recording lockouts and reboot persistence.

**Hardware-dependent behavior remains NOT VERIFIED** until tested on the actual Feather ESP32 V2 and router. Required bench checks include real AP+STA association, iPhone access through both interfaces, bad-password behavior, router power-off/recovery, RSSI reporting, SW38 authorization, reboot persistence, and a real 50 Hz temporary recording while the router/iPhone link is interrupted.

