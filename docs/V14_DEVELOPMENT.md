# Developer, GitHub and Release Guide

## Project map

- [web/](../web/) — classic blue HTML, responsive CSS, offline PWA, Sensor Lab, cloud and GIS1 analyzer
- [Green_Ion_V14/](../Green_Ion_V14/) — Feather ESP32 V2 autonomous flight recorder and local backup dashboard
- [Ground_Receiver/](../Ground_Receiver/) — Uno R3 radio receiver, USB telemetry
- [libraries/](../libraries/) — shared radio and original RKT4/GIS1 record formats
- [tests/](../tests/) — Python/C++/Node regression tests
- [.github/workflows/build-v14.yml](../.github/workflows/build-v14.yml) — builds both firmware images, tests, packages ZIP + SHA256
- [Past_Versions/](../Past_Versions/) — historical source, V13 README and rollback links
- [docs/](./) — specialized help pages

## Recommended workflow

1. Branch from main using a named V14 feature branch; review changes as a PR. Never rewrite historical tags.
2. Keep optional features as individual files and toggles. Preserve working RKT4/GIS1 formats and onboard autonomous recording.
3. Run Python regressions (python3 -m unittest discover -s tests), Node tests (node --test tests/v14_*.test.mjs), and the GitHub Actions Feather/Uno compile.
4. Review Vercel preview; merge only after CI. Vercel deploys production automatically from main.
5. GitHub publishes a complete ZIP, Feather .ino.bin, Uno .ino.hex and SHA256 manifest. OTA installation is **never automatic**.
6. Test on the actual Feather and Uno before field use.

## Optional feature map

| Module | Feature | Disable under System |
|---|---|---|
| v14_sensor_lab.mjs | Live sensor diagnostics | Sensor Lab |
| v14_sensor_history.mjs + analysis/gis1.mjs | Historical IMU/GPS graphs and GeoJSON | SEN analysis |
| v14_cloud.mjs | Cloud manual/automatic backups | Auto-sync toggle (manual remains) |
| v14_layout.css | Responsive / compact CSS | Compact toggle |
| v14_sensor_catalog.mjs | Display units | Imperial toggle |

For full code removal, also remove the matching import, HTML card, tests and service-worker cache entries; leaving a missing import would break the PWA.

Sources: [GitHub Actions](https://docs.github.com/en/actions), [Vercel Git integration](https://vercel.com/docs/git), [Supabase Access Control](https://supabase.com/docs/guides/storage/security/access-control).
