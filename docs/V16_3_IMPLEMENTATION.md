# GREEN ION V16.3 implementation and V16.2 completion audit

Baseline: V16.2 stable release `v16.2.0`, main commit `041af441901b8b0f2e2b17bc23db144465a3267e`.

Status vocabulary: **COMPLETE**, **PARTIAL**, **MISSING**, **BLOCKED**, **REQUIRES PHYSICAL TEST**.

## V16.2 completion matrix

| Requirement | Status | Evidence / V16.3 action |
|---|---|---|
| Feather V16.2 firmware generation | COMPLETE | `Green_Ion_V16_2`; compiled and released BIN. |
| Compatible Uno V16.2 receiver | COMPLETE | `Ground_Receiver_V16_2`; protocol v2 / 70 bytes retained; released HEX. |
| Windows reconnect / COM-port recovery | COMPLETE | Automatic Feather retry and Uno rediscovery retained. |
| Exactly five tabs / #081420 | COMPLETE | Regression-locked. |
| Balanced everyday controls | COMPLETE | Balanced is default; Simple/Advanced affect presentation only. |
| Live Dashboard rocket | COMPLETE | Sensor-based/estimated/illustrative Canvas2D visualization with stale freeze and replay. |
| Genuine WebGL 3D | COMPLETE | Perspective/depth WebGL renderer; real browser shader/readback smoke passes. |
| 3D rocket geometry | COMPLETE | Nose/body/fins geometry rendered in WebGL. |
| Timestamp replay / speed / jumps | COMPLETE | Actual timestamps, 0.25×–5×, launch/apogee/descent/landing jumps. |
| Camera / touch controls | COMPLETE | Orbit/pan/zoom/follow/ground/top/side/free. |
| Graph synchronization | COMPLETE | Replay events synchronize graph/3D time. |
| Embedded Google Maps | COMPLETE | Optional configured browser key, confirmed GPS only. |
| Offline map fallback | COMPLETE | Existing offline track/coordinates/distance/bearing/external Maps link preserved. |
| Safe SD manager / backup evidence | COMPLETE | Completion check, backup evidence, exact filename, companions first/BIN last, recheck. |
| Flight library / comparisons | COMPLETE | Search/annotations retained; multi-flight worker comparison added. |
| Guided testing / diagnostics | COMPLETE | One primary Full Home Test and System Health. |
| Windows startup/update/rollback | COMPLETE | Verified side-by-side package and persistent user data. |
| Phone/tablet improvements | COMPLETE | Responsive/touch surfaces; no claim of direct iPhone LoRa. |
| Supabase / data integrity | COMPLETE | Local-first originals, checksums, retry-safe optional cloud. |
| Data-quality scorecard | COMPLETE | Advisory data-quality summary; not flight authorization. |
| Rocket profiles / battery reporting | COMPLETE | Profile metadata + measured battery reporting without unsupported endurance precision. |
| First-run / accessibility | COMPLETE | Optional first-run and focus/touch improvements. |
| Protected OTA / rollback | COMPLETE | Deliberate bench-only firmware update remains separate from app update. |
| Automated software tests | COMPLETE | Python, Node, WebGL browser smoke, firmware compile, packaging and release smoke. |
| V16.2 documentation | COMPLETE | Setup, firmware, receiver, LoRa, SD, Maps, 3D, visualization, validation guides. |
| V16.2 release assets | COMPLETE | GitHub release `v16.2.0` targets the verified merged commit. |
| Real shared SD + LoRa SPI proof | REQUIRES PHYSICAL TEST | V16.3 hardens architecture and adds a measured bench procedure; CI cannot certify perfboard behavior. |
| Real RF/GPS/power/flight validation | REQUIRES PHYSICAL TEST | Remains in hardware evidence ledger. |

## V16.3 work

- [ ] Centralize shared SPI ownership and defer RFM9x DIO0 SPI work out of interrupt context.
- [ ] Give sensor sampling / BIN / checkpoints / SEN priority over radio.
- [ ] Add shared-SPI diagnostics and bounded radio recovery evidence.
- [ ] Extend Full Home Test with concurrent SD + LoRa evidence and actual Windows Uno receipt when available.
- [ ] Add host fault-injection / scheduler / protocol regression tests and physical perfboard stress procedure.
- [ ] Improve radio recovery without reset storms or recorder interaction.
- [ ] Simplify the five existing tabs further without changing visual identity.
- [ ] Verify/improve WebGL, live rocket, Maps, library/report and mobile behavior without replacing working V16.2 code.
- [ ] Improve power/reset/sensor recovery evidence.
- [ ] Add `docs/V16_3_SHARED_SPI_PERFBOARD.md`.
- [ ] Update Windows/app/firmware identities to V16.3 and preserve rollback.
- [ ] Full CI → PR → main → verified `v16.3.0` release if all mandatory software gates pass.

## Nonnegotiable compatibility

- RKT4 v2 BIN unchanged.
- GIS1 SEN unchanged.
- LoRa protocol v2 remains 70 bytes unless a measured need proves otherwise.
- Ground LoRa receive-only.
- Autonomous recording remains authoritative.
- No ignition, staging, deployment, pyrotechnic or guidance controls.
- `#081420` and exactly five visible tabs remain regression-locked.
- Physical hardware results are never inferred from CI.
