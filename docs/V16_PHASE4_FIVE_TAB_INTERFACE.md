# GREEN ION V16 Phase 4 — Five-tab interface

## Goal

Reduce the top-level dashboard from thirteen tabs to five without deleting working tools.

The V16 primary navigation is:

1. **Mission** — mission workflow, flight status and measured preflight.
2. **Live** — Wi-Fi/LoRa links, live field telemetry and receiver diagnostics.
3. **Recovery** — last confirmed GPS, recovery state and locate tools.
4. **Data** — local flight history, SD files and optional cloud backup.
5. **System** — device status, utilities, updates and help.

## Compatibility

Existing page IDs and feature code remain in place. Specialized V14/V15 pages are contextual subpages, not top-level navigation. This keeps old internal links and hardware controls working while making normal launch-day use less cluttered.

The five primary tabs are the only visible top-level tabs. A contextual shortcut card inside each group exposes its related detailed pages.

## Navigation behavior

Legacy pages map to a primary group:

- Flight and Testing → Mission
- Field and Connections → Live
- Storage, History and Cloud → Data
- Utilities, System, Updates and Help → System
- Recovery remains Recovery

Programmatic navigation from preflight, file analysis, recovery and update flows still uses the original page IDs. The top-level highlight follows the group automatically.

Keyboard shortcut support: Alt+1 through Alt+5 opens Mission, Live, Recovery, Data and System.

## Validation boundary

This phase changes browser navigation only. It does not change Feather autonomous recording, flight detection, LoRa directionality, RKT4/GIS1 formats, OTA rules or hardware behavior.
