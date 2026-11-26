# GREEN ION V18 — So Long electronics installation steps

**Owner:** So Long Build Helper · **Assignment:** V18-NEXT-BUILD-20261121 · **Status:** PROVISIONAL / NOT TESTED / NOT FLIGHT-READY

**Audience:** beginner assembling the Estes Pro Series II So Long and preparing a removable GREEN ION recorder/recovery payload. **Do not install motors or igniters during any step in this guide.** This guide complements, rather than duplicates, [HARDWARE_PLAN.md](HARDWARE_PLAN.md) and the [12-step physical qualification checklist](PHYSICAL_DEV_TEST_CHECKLIST.md). GREEN ION is a passive recorder/recovery payload; it must not control ignition, staging or ejection.

## 0. Stop-work rules (read first)

**STOP immediately** for an undamaged/unknown antenna mismatch before any RF transmission; torn radio pad or cable; LiPo swelling, puncture, overheating, exposed leads, reversed JST polarity or short; tube crushing, obstructed stage joint/streamer or unknown free clearance; pin/power disagreement; uncertain battery reserve; unreadable/missing/incomplete SD recording; unverified sleep/wake behavior; or unresolved motor/CG/stability issue. Remove power, keep motors and igniters away, photograph the issue, and ask an experienced mentor or the Integration Manager before restarting. **No drilling static ports, buying sized parts, permanent sled mounting, RF TX, firmware flashing or flight approval on the basis of this document.**

## 1. Finish body-tube preparation BEFORE electronics fitting

1. Let existing Rust-Oleum 2-in-1 filler/sandable primer dry per the exact can instructions. Lightly sand exterior with fine paper (e.g. 400 grit), keeping primer in the spiral grooves rather than cutting into cardboard. Use dust/eye protection and ventilation.
2. If spirals remain, apply another *light* compatible coat; cure and lightly sand again. Do **not** use lacquer thinner as a default method on this sprayed product.
3. Mark fin/root and other *structural glue areas* per the official Estes kit instructions. Before applying glue, sand **only those contact areas** back to clean bare paper/cardboard; remove dust. Keep coupler fit surfaces, inner tube, and recovery path free of overspray and abrasive damage.
4. Complete the kit’s structural assembly according to Estes before committing an electronics design. Do not alter stage joints, motor mounts, ejection path or recovery system for payload convenience.

**Gate:** all adhesive joints bond to manufacturer-specified material; tube/coupler still fit correctly. Finishing primer does not prove mechanical flight safety.

## 2. Measure the ACTUAL free envelope (no assumed internal dimension)

The published **31 mm is OUTSIDE diameter**. The nominal 8-inch payload body-tube length is **not** usable electronics length. Measure clear inside diameter with a caliper at *front, middle, and rear*, along two perpendicular directions; use the smallest safe reading. Measure free axial length with nose shoulder, aft coupler/retention, shock cord and packed recovery present. Photograph interference and reserve room for finger access, bend radii, SD/USB access, and clean stage/recovery separation.

### Fill-in measurement sheet — leave blank until measured

| Measurement (record units) | Actual reading | Evidence / notes | State |
| --- | --- | --- | --- |
| Clear ID front — axis A / B (mm) | ___ / ___ | ___ | NOT TESTED |
| Clear ID middle — axis A / B (mm) | ___ / ___ | ___ | NOT TESTED |
| Clear ID rear — axis A / B (mm) | ___ / ___ | ___ | NOT TESTED |
| Minimum unobstructed ID (mm) | ___ | Include seam/glue bulges | NOT TESTED |
| Payload tube physical length (mm) | ___ | Measure actual, do not substitute nominal | NOT TESTED |
| Nose shoulder intrusion (mm) | ___ | Installed position | NOT TESTED |
| Aft coupler/retainer intrusion (mm) | ___ | Installed position | NOT TESTED |
| Shock cord/recovery reserved axial envelope (mm) | ___ | Fully packed | NOT TESTED |
| Usable clear length, after ALL intrusions (mm) | ___ | Confirm with physical template | NOT TESTED |
| Feather + USB/JST plugged envelope L×W×H (mm) | ___ | Actual plugs and bend clearance | NOT TESTED |
| BMP388 / IMU board(s) + harness (mm) | ___ | Chamber position | NOT TESTED |
| microSD reader + card removal envelope (mm) | ___ | Card-access clearance | NOT TESTED |
| GPS patch + lead envelope (mm) | ___ | Sky-view obstruction | NOT TESTED |
| LoRa board + antenna/feed envelope (mm) | ___ | Connector/strain relief | NOT TESTED |
| 350 mAh LiPo actual L×W×T, mass, condition | ___ | Label/polarity photo | NOT TESTED |
| Complete dry-fit mass and center of gravity | ___ | Each intended configuration later | NOT TESTED |
| Proposed static chamber effective volume (cm³) | ___ | Separate from ejection gases | NOT TESTED |

**Stop here** if any physical reading is unavailable. Do not manufacture a dimensioned, permanent sled from the 31 mm published outside diameter.

## 3. Scale worksheet for a PROVISIONAL removable sled

Draw at **1:1** on graph paper or CAD **only after measurements**, then print a 100 mm calibration line and verify it with a ruler before trusting the template.

| Sled dimension / calculation | Fill in |
| --- | --- |
| Minimum measured free ID, `D_min` | ___ mm |
| Clearance allowance each side, `C_side` (chosen from safe sliding dry-fit, not guessed as verified) | ___ mm |
| Maximum proposed total assembly width = `D_min − 2 × C_side` | ___ mm |
| Actual widest *assembled* PCB + standoff + wiring envelope | ___ mm |
| Minimum measured usable axial length, `L_free` | ___ mm |
| End clearance and recovery/service reserve, `C_front + C_rear` | ___ + ___ mm |
| Maximum proposed sled length = `L_free − C_front − C_rear` | ___ mm |
| Component positions from chosen sled datum (Feather / sensors / SD / GPS / radio / LiPo) | ___ / ___ / ___ / ___ / ___ / ___ mm |
| USB-C access, SD extraction, JST disconnect and button-actuation room | ___ |
| Retention point, anti-rotation feature, nonconductive standoff thickness | ___ |
| Antenna fixed route and strain-relief points, isolated from recovery | ___ |
| Required proof: cardboard dummy inserted/removed 10× with fully packed recovery | PASS / FAIL / NOT TESTED |

The outline is **provisional even once numbers are entered**: measure cable bend radii, PCB fasteners and powered equipment envelopes, then dry-fit a cheap nonconductive cardboard dummy before printing/cutting hardware. If interference exists, redesign or choose a different airframe—never compress the LiPo or block recovery. Label actual dimensions and template revision on every sketch.

## 4. Lay out electronics on a removable, insulated backbone (dry fit only)

1. Start with a **nonconductive** axial backbone, fixed standoffs, anti-rotation stop, and positive captured retention that does not weaken the thin tube or intrude into coupler/streamer travel. No tape-only/hot-glue-only structural attachment.
2. Place the **Feather ESP32 V2** where USB-C, battery JST and programming access remain possible. Keep the existing **3.7 V 350 mAh LiPo** only if undamaged and bench-qualified; use a smooth padded pocket and separate nonabrasive positive strap. No fastener or tie presses on the pouch.
3. Mount **BMP388** where its sensing opening faces an isolated, *externally vented* static-air chamber, not the sealed foam or ejection-gas volume. Mount the **IMU** rigidly with axes labeled. Group sensor cables without obstructing the port.
4. Orient **microSD** so its card can be removed without yanking wires. Preserve original flight files and leave SD-first recording electrically and mechanically prioritized.
5. Give the **GPS patch** an unobstructed view through the final airframe placement, keeping it away from large metal parts and the battery. Position **LoRa radio and frequency-matched antenna** with mechanically secured feed/strain relief, away from conductive parts, separated joints and recovery. Damaged/loose antennas are a **NO-TX** condition.
6. Add labeled insulated power/I²C/SPI harnesses, short service loops, sleeved solder joints and strain relief; no bare pads touching fasteners or battery. Keep Feather 3.3 V logic compatible and reconcile wiring against the live V18 firmware source **before soldering**.
7. Verify physical SD card extraction, USB-C plug insertion, battery disconnect, antenna clearance, and nose/coupler/recovery free movement with the entire mock assembly fitted.

**GPIO/wake dependency:** Existing onboard **SW38/GPIO38** is a Wi-Fi management button, not verified as an ESP32 deep-sleep wake source. Primary's recovered design at [PR #63](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/63) explicitly leaves `recovery_deep_sleep_enabled=false` and `recovery_wake_hardware_verified=false`. Do **not** wire a guessed alternate pin, advertise deep-sleep wake, cut an access hole or simulate a successful wake. Await a versioned pin/power/wake contract jointly approved by Primary and Integration Manager.

## 5. Pressure ports, staging and recovery are hard physical gates

Keep the BMP388 chamber isolated from ejection gases while sampling exterior static pressure. Measure the **actual effective chamber volume**, identify smooth sidewall space clear of shoulders/couplers, then seek an altimeter-manufacturer-based port count/diameter/location design and mentor/RSO review. **Do not drill now**, and do not copy vent sizes from a larger rocket. When approved, deburr ports and demonstrate repeatable slow pressure/elevation response. No sensor reading from the previously unported ROCstock setup qualifies this design.

Keep electronics, RF antenna, wires, battery and retaining hardware out of streamer, shock cord, nose separation and booster/upper-stage interfaces. Do not modify staging or ignition mechanisms. Document real packed recovery clearance, stage connection/retention, measured loaded mass/CG, full-stack and postseparation stability analysis, and experienced RSO sign-off *before any future flight*. A software CI pass is not physical sign-off.

## 6. Motor-free, no-igniter TEST sequence (record evidence; none executed yet)

Use the existing [12-step checklist](PHYSICAL_DEV_TEST_CHECKLIST.md) for exact pass/fail categories; use this sequence to capture installation-specific evidence. For each row, enter **PASS / FAIL / NOT TESTED**, date, instrument/log/photo and observed values. **Default all rows NOT TESTED.**

| Order | Safe bench procedure / evidence to retain | Initial result |
| --- | --- | --- |
| 1 | Photograph all boards/antennae and LiPo. **Unpowered** inspection for shorts, solder bridges, pad damage, loose contacts, JST polarity; verify continuity only with power disconnected. | NOT TESTED |
| 2 | Inspect power rail correctness, then power with approved safe USB source; record current/voltage and heat; separately verify known-good battery connector orientation. | NOT TESTED |
| 3 | Read stationary BMP388 baseline over time and slowly rotate IMU axes; capture raw values and repeatability. No claimed numeric tolerance without approved reference. | NOT TESTED |
| 4 | Check pressure chamber layout and **provisional** port location without drilling; after mentor approval, controlled pressure response and repeatability against reference. | NOT TESTED |
| 5 | Use an explicitly reserved **temporary-only** SD test path; verify no real files are removed. Perform safe simulated recording, reboot/readback and **two separate saved bench flights** with distinct intact BIN/TXT records. Capture filenames, counts, sizes, verification/finalization status; any false SAVED or missing metadata = FAIL. | NOT TESTED |
| 6 | Measure boot, pad standby, armed, SD-writing, Wi-Fi changes and recovery-state current/voltage with appropriate meter. Run a supervised representative-duration soak against agreed mission time + reserve; record minimum voltage, brownouts/resets and cell temperature. **350 mAh endurance is not proven.** | NOT TESTED |
| 7 | With **no RF TX**, inspect both rocket and Uno RFM95 board/antenna matching, pads and 3.3 V/5 V compatibility; verify passive radio SPI identity/init as supported. | NOT TESTED |
| 8 | Connect Arduino Uno via USB to Windows Chrome/Edge Web Serial; document permission chooser, valid/invalid packet presentation and unplug/reconnect while staying RX-only. | NOT TESTED |
| 9 | **Only after antennas on both ends are repaired, secured and matched, with documented radio/legal/range authorization:** conduct a deliberately approved controlled packet/range test with timestamps/RSSI/SNR and GPS freshness. Until then RF TX stays disabled. | NOT TESTED |
| 10 | Verify physical SW38 button access *mechanically* without cutting. Actual Wi-Fi wake from any sleep state only after Primary/Integration approve GPIO and firmware state and authorize a motor-free test. | NOT TESTED |
| 11 | Fit/unfit dummy sled at least 10×, then inspect secured components, electrical continuity, SD/USB clearance and free streamer/shock-cord/nose/stage movement. | NOT TESTED |
| 12 | Measure finished weights and CG for exact intended rocket configurations, assess full and post-stage stability with validated simulation and mentor/RSO; **no flight-ready conclusion here**. | NOT TESTED |

**Safety boundary:** No physical tests were conducted by this documentation author. Firmware recovery *policy* currently models a 5-minute recovery window and 3-minute scheduling interval; actual TX, deep sleep and wake are **disabled/unverified**. Hardware changes and live RF require separate approval. Never allow GPS/LoRa/Wi-Fi diagnostics to interrupt SD-first 50 Hz RKT4 v2 recording.

## 7. Handoff / evidence needed from the builder

Provide to PR #61/Integration Manager #60: completed measurement table and dated caliper photos; true device envelopes including connectors, sled dummy-fit photos and battery condition; before/after antenna close-ups; wiring inspection, bench logs/BIN/TXT filenames, saved-file checks, recorded currents/voltages/duration, serial/RX evidence; proposed static-chamber volume and mentor-approved port drawing; packed recovery clearance, stage fits, masses/CG and simulation. Mark all absent entries **NOT TESTED**, not PASS.

**Do not duplicate** hardware qualification tables in [HARDWARE_PLAN.md](HARDWARE_PLAN.md) or [PHYSICAL_DEV_TEST_CHECKLIST.md](PHYSICAL_DEV_TEST_CHECKLIST.md). The Integration Manager should link this installation walkthrough in the V18 bench guide when reviewed, and Primary should resolve the SW38/GPIO38 wake/deep-sleep hardware limitation before any hole/pin change.

**Sources:** [Estes So Long instructions](https://estesrockets.com/cdn/shop/files/Estes-So-Long-Instructions.pdf?v=1722444368), [Estes So Long product](https://estesrockets.com/products/so-long), [Primary firmware PR #63](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/63), [Integration PR #60](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/60), [hardware issue #59](https://github.com/aidenjantzi1-ctrl/GREEN-ION/issues/59).
