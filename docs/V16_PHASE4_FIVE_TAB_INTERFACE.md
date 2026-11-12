# GREEN ION V16 Phase 4 — Five-tab interface

## Goal

Make normal launch-day use simple without deleting the advanced tools already built.

The only visible top-level tabs are:

1. **Dashboard**
2. **Flight**
3. **Flights & Analysis**
4. **Testing**
5. **Settings & Advanced**

The old page IDs and hidden tab aliases remain for compatibility with older code paths, but they are not visible top-level navigation.

## Dashboard

Dashboard is the quick status page. It keeps only the information that matters immediately:

- mission state;
- rocket and motor;
- active Wi-Fi/LoRa source;
- GPS status;
- SD/recording status;
- altitude, vertical speed, battery and other compact live metrics;
- **Prepare for Flight** shortcut.

Detailed live graphs and Sensor Lab are moved into one expandable **Detailed live telemetry & graphs** section.

## Flight

Flight is the operational workflow:

- V16 Mission Controller;
- rocket/motor profile;
- preparation and recorder health;
- field controls;
- recovery tools and last-known position.

The mission controller stays first. Recovery is part of the same flight workflow instead of being a separate top-level tab.

## Flights & Analysis

This combines storage and history:

- SD/local completed-flight files;
- local immutable originals;
- RKT4/GIS1 analysis;
- advanced reports;
- sensor-history analysis;
- comparisons/exports;
- local archive/restore.

It remains usable offline because it works from locally saved originals.

## Testing

Testing remains its own top-level destination because it is a common bench workflow. It includes:

- one-button full preflight;
- hardware tests;
- microSD checks;
- calibration and recording verification.

## Settings & Advanced

Advanced features are grouped into expandable sections:

- Profiles, sensors & recording;
- Utilities & controls;
- Connections, radio & diagnostics;
- Cloud backup;
- Firmware & app updates;
- Help & troubleshooting.

They are still present, but no longer occupy separate top-level tabs.

## Compatibility behavior

Legacy destinations map to the new visible tabs:

- Field / Recovery → Flight
- Storage / History → Flights & Analysis
- Utilities / Connections / Cloud / Updates / Help → Settings & Advanced
- Testing → Testing
- Status → Dashboard

The hidden v16LegacyTabs aliases preserve older programmatic links while the visible navigation stays exactly five tabs.

Alt+1 through Alt+5 opens Dashboard, Flight, Flights & Analysis, Testing and Settings & Advanced.

## Responsive and offline behavior

The five tabs remain horizontally scrollable on narrow phones and use touch-sized controls. Dashboard summary, mission controller and important analysis cards remain full-width where useful on desktop. Advanced groups collapse cleanly rather than producing long empty grid rows.

Both V16 layout/navigation modules are included in the offline PWA cache.

## Validation boundary

Phase 4 changes browser organization only. It does not change Feather launch detection, autonomous SD recording, LoRa directionality, RKT4/GIS1 formats or hardware behavior.
