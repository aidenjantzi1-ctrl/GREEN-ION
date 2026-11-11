# GREEN ION V14 Architecture and Safety Boundaries

- **Feather ESP32 V2:** autonomously measures, calibrates and records RKT4/GIS1 data to SD, transmits receive-only LoRa telemetry and serves a blue local fallback UI from 192.168.4.1. OTA maintenance is explicitly guarded.
- **Hosted blue PWA:** static Vercel app; renders all legacy dashboard tabs, offline History and Cloud backup, GitHub release prompts, Web Serial on desktop, source/freshness indicators. No telemetry or flight controls route through Supabase.
- **Browser local Wi-Fi:** HTTPS-to-HTTP private network access must be confirmed on each platform. The fallback Feather-hosted UI works without internet and does not rely on browser CORS exceptions.
- **Windows Uno:** Web Serial 115200 reader for JSON line telemetry; no serial transmitter/control commands. No Uno USB support assumed on iOS.
- **Local recordings:** IndexedDB preserves BIN/SEN/TXT originals, SHA-256 integrity and backup/restore. Browser storage can be evicted: export a copy.
- **Supabase:** Auth email accounts, table public.flight_backups with RLS by auth.uid(), private storage bucket green-ion-flights with per-user folder policy. Cloud sync is optional and deferred during offline operation.
- **GitHub:** all compiled .ino.bin/.hex and ZIP assets produced in Actions, SHA256 manifest, beta release after passing tests. Website checks only repository-owned release links and asks before download.
- **No cloud OTA or remote flight commands:** Even if Supabase or Vercel is compromised or unreachable, it cannot arm the rocket. OTA uses guarded local Feather maintenance with physical supervision.

## Software validation versus hardware

GitHub CI performs parser, host, receiver-protocol and board build checks. They cannot replace Feather, SD, LiPo, GPS, IMU, real LoRa RF, browser local-network, or real OTA bench checks. No physical devices are touched by GitHub deploys.

## Version archive strategy

Historical V10/V11 live under Past_Versions. Preserve full V12 and V13 source/release tags while V14 beta is being hardware validated. Do not delete old source or release artifacts merely to make the root smaller. Older documents are linked in Past_Versions/README.md.
