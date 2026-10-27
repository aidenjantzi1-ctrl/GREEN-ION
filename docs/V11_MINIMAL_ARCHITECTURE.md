# V11 recorder/browser responsibility map

## Feather keeps

- BMP388 discovery, validation, pressure and temperature sampling.
- Monotonic timestamps and 10/25/50 Hz scheduling.
- Battery sampling and low-power rate protection.
- Calibration, preflight checks, raw RKT4 writes and permanent numbering.
- Buffering, alternating checkpoints, emergency spill buffer and interrupted-flight recovery.
- Minimal pressure-derived launch/landing state required for autonomous start/stop.
- Compact live status, configuration, control, raw BIN/metadata downloads, OTA and rollback.

## Browser owns

- Presentation filtering and detailed altitude, velocity, acceleration and G estimates.
- Full postflight phase interpretation, graphs, replay and CSV/printable reports.
- IndexedDB original-BIN history, SHA-256 checksums, deduplication, search, comparison, backup and restore.

V11 removes Feather-side derived CSV generation and the redundant derived postflight report. `/api/csv` remains as a lightweight compatibility response directing clients to download the immutable BIN. The embedded dashboard now decodes BIN and generates CSV in phone JavaScript. Essential phase values retained in firmware exist only to control autonomous recorder operation and preserve compact recovery metadata.
