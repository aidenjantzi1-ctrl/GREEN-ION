# V18 independent development-test evidence

Status: software-only, synthetic validation. This report does not authorize a flight, hardware connection, release, or merge to `main`.

## Source and ownership snapshot (2026-11-18)

| Source | Observed SHA | Ownership used here |
| --- | --- | --- |
| Frozen `main` | `17f517f275be7b52e3e71a1f2d656edbc65a08ea` | V17.0.1 RKT4 and parser baseline |
| Integration PR #60 | `96291b0661fe6df25df52cdd4b7998a03911c071` | Preview launcher, bridge, packaging, combined contracts |
| Secondary PR #58 | `5f14c2656eba4f9802cb64969e2765b716161716` | V18 four-tab app, offline helpers, Web Serial |
| Primary firmware branch | `ca1df3683155430210c2c99cb84cc37504d77631` from a fresh Git fetch | Feather source and firmware diagnostics; PR #63 metadata may lag moving branch |
| Audit PR #62 before this package | `e08412cd8ed32a1c179ad12ca53ac108185ccbda` | Read-only RKT4 audit |

This package changes only the audit CLI and independent test/evidence files. Integration already stages an older audit snapshot. The Secondary app, Primary firmware, and Integration launcher/bridge/package remain owned by their workers.

## Run without hardware

From the repository root, with Python 3 and Node.js installed:

```powershell
$env:PYTHONUTF8='1'
python -m unittest -v tests.test_v18_independent_audit tests.test_v18_rkt4_audit
node --test tests/v18_contract_probe.test.mjs
```

The Python corpus generates synthetic bytes in memory or temporary directories. It has no actual flight measurements. The contract probe does not call HTTP, serial, LoRa, or firmware commands. On audit PR #62 alone, the V18 UI core is absent and the Node test explicitly reports **NOT TESTED**. To exercise the actual V18 helper, run the read-only probe against a reviewed integration checkout:

```powershell
node tools/v18_contract_probe.mjs --source-root "C:\path\to\reviewed-integration-copy"
```

It outputs a source SHA when Git access is available, PASS/FAIL/NOT TESTED for each software contract, and exits 1 for any FAIL or 2 when the V18 core is unavailable. The JSON omits local paths and synthetic GPS coordinates. Confirm the source SHA before using results as integration evidence.

## Audit copies of original files

**Copy**, keeping each BIN with its matching TXT/JRA/JRB/RCV sidecars in a separate folder. Keep originals untouched. From the repository root:

```powershell
python tools/v18_rkt4_audit.py --summary "C:\path\to\copied-flights"
```

The default command without `--summary` still emits the original JSON array. `--summary` adds a versioned JSON object with status counts, per-file leaf names and issue codes. It does not include full paths or GPS coordinates. Exit 0 requires every BIN to be `structurally_complete`; exit 1 means at least one file is incomplete, invalid, or unverified; exit 2 means an input has no BIN files. For a mixed batch, inspect **every** file result. `structurally_complete` is a limited structural PASS. `invalid` or `incomplete` is FAIL/investigate; `unverified` or no BIN is NOT TESTED/insufficient evidence. RKT4 v2 has no per-record checksum, so none of these statuses proves sample content or altitude accuracy.

## Test matrix and limits

| Scenario | Software evidence | Physical evidence still needed |
| --- | --- | --- |
| Header, rate, 32-byte/16-byte layout, 50 Hz synthetic records | Deterministic Python corpus, JS parser parity for comparable layout | Real BMP388/IMU timing under load |
| Complete, missing/bogus TXT, whole/partial truncation, pending journals/recovery | Audit status and issue codes, file hash unchanged | Power interruption and SD readback on disposable card |
| Two distinct flights, one bad file, lower-case BIN, empty folder | Batch summary counts and nonzero exit | Two sequential motor-free recordings without reboot |
| Timestamp duplicate/backward/wrap/gap and invalid sensor fields | Synthetic corpus retains raw bytes; warnings/issues only | Properly vented barometer and real motion traces |
| Firmware status sequence, stale bridge, critical SD vs optional GPS/LoRa | Offline V18 helper probe; no live commands | Feather status freshness and actual SD/sensor behavior |
| Receiver malformed/unknown protocol, stale fix, missing coordinate | Offline protocol-v2 helper probe | Uno USB permission/reconnect and antenna-safe RF bench |
| Wi-Fi state and save verification | Primary source contract and separate firmware CI only | Measured Wi-Fi transitions, battery draw, safe completion on hardware |
| Windows no-hardware BIN import | Secondary automated browser tests only | Extracted Windows ZIP, Chrome/Edge offline restart and permission smoke |

The browser RKT4 parser decodes complete records and warns on unexpected sample rate or partial tails. The Python audit also reads matching completion/recovery sidecars and treats unsupported rate/record values as invalid. These are intentionally different scopes; neither should rewrite raw bytes or invent samples.

## Reproducible cross-owner findings at integration SHA `96291b0`

- `web/v18_offline_app.mjs:76-77`: after any HTTP 200 metadata body, `verifySave()` sets `saveVerified=true` if `/api/files` claimed completion. A blank or wrong-flight TXT therefore can permit SAVED/FLY AGAIN. The probe executes the isolated app function with wrong-flight, incomplete metadata and reports `save_metadata_binding: FAIL`. Reproduction and requested Secondary fix: PR #58 comment [save evidence](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/58#issuecomment-6984534837).
- `web/v18_offline_core.mjs:7,128`: `Number(null)` becomes zero, so a null battery reading becomes a false 0 V FAIL rather than NOT TESTED.
- `web/v18_offline_core.mjs:7,26-28`: a single null latitude or longitude becomes zero and may be labeled LIVE when the other coordinate is present. The offline probe reported **8 PASS / 3 FAIL** across all three cases at the exact Integration SHA. Reproduction and requested Secondary fix: PR #58 comment [null evidence](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/58#issuecomment-6984630983).

These findings are software helper behavior on synthetic inputs. They do not imply that any physical flight file was corrupted or that real GPS hardware produced those values. Integration should retain DEV-TEST BLOCKED for the affected save/GPS/battery evidence until the Secondary branch fixes and the combined exact-head tests pass.
