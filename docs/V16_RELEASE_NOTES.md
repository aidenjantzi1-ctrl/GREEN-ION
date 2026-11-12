# GREEN ION V16.2

GREEN ION V16.2 preserves the existing dark-blue five-tab application and autonomous recorder while upgrading both the application and hardware firmware generation.

## Highlights

- **Feather V16.2:** explicit firmware/session/reset identity, stronger SD/sensor diagnostics and safe idle recovery without changing RKT4 v2 or GIS1.
- **Uno V16.2:** explicit receiver version; receive-only protocol v2 remains fixed at 70 bytes.
- **Windows reliability:** brief status delays keep last-known evidence visible while hardware commands immediately require fresh verification.
- **Clean Dashboard:** Balanced default, smart next action and a polished sensor-based/estimated 2D rocket visualization with clearly illustrative effects.
- **True 3D:** WebGL perspective/depth, real ENU trajectory geometry, generic 3D rocket, interactive cameras, timestamp replay, phase jumps, point selection and synchronized analysis.
- **Recovery:** optional embedded Google Maps with confirmed raw GPS trail and existing offline fallback.
- **SD safety:** completion state plus browser/Windows/cloud evidence before exact-name deletion; interrupted/recovery evidence is retained.
- **Flights & Analysis:** rocket/motor/date/status/backup filters, local annotations, reports, data-quality scorecard and multi-flight comparison.
- **Testing:** one Run Full Home Test front door plus expanded System Health and human-readable troubleshooting.
- **Packaging:** verified Windows ZIP/SHA/manifest, offline web ZIP, Feather BIN, Uno HEX, complete V16.2 source ZIP and release SHA manifest.

## Important limits

The rocket visual, flame, parachute and ground-contact depictions are not physical confirmation. Relative gyro attitude is not absolute attitude. GPS/radio failure never blocks primary SD recording.

Real RF range, physical sensor behavior, SD timing, LiPo/brownout behavior, wiring/mechanical installation, OTA interruption/rollback and real flight remain **NOT TESTED** until measured on the assembled hardware.

See `docs/V16_2_WINDOWS_SETUP.md`, `docs/V16_2_FIRMWARE_UPDATE.md`, `docs/V16_2_UNO_RECEIVER.md`, `docs/V16_2_GOOGLE_MAPS.md` and `docs/V16_2_HARDWARE_VALIDATION.md`.
