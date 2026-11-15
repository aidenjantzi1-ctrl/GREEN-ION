# GREEN ION V18 — Estes So Long hardware integration and qualification plan

**Status: PROPOSED / NOT FLIGHT-READY — measurements and real hardware tests missing.**  
**Owner:** V18 So Long Build Helper; **integration review:** V18 Integration Manager.  
**Scope:** physical installation, harness, radio/antenna, pressure, power, mechanical and home verification. **No firmware/frontend changes.** GREEN ION records flight data and supports recovery; it does not control ignition, stages, steering or ejection.

## A. Verified constraints and open decisions

- Estes Pro Series II So Long (009722): overall length **46.2 in (117.3 cm)**, published **outside** diameter **1.21 in (31 mm)**, manufacturer product-page nominal weight **5.75 oz (163 g)**, maximum claimed altitude 3,600 ft, dual streamer recovery. The manual separately gives estimated **6.3 oz (178.6 g)**: **measure actual complete rocket mass**, do not choose either published value for stability.
- Manufacturer-approved **single-stage** motors: **E16-6, E16-8, F15-6, F15-8**. For **two stages**, Estes lists upper **E16-8 / F15-8** and booster **E16-0 / F15-0**. These are approved *stage motor choices*, not unconditional approval of every combination after payload mass changes; match kit/manual, manufacturer pairing and RSO.
- Manual includes nominal **8 in payload-section body tube**, but nose shoulder, aft coupler/pin and intrusions consume space; **usable clear length and internal diameter are UNKNOWN**. 31 mm is the **outside diameter**, not usable width.
- Estes manual stock CG bands cited for comparison: **27–32 in from nose tip single stage; 33–37 in two stage**. These do not by themselves prove stability of a modified vehicle. Simulate full stack at ignition and upper stage after separation with measured components and motors; review CP/CG margins and recovery; get RSO approval.
- V17 source hardware contract is the current **baseline**, not an assertion that the V18 build is tested. SD-first RKT4 v2 at 50 Hz is the priority. GPS/LoRa/Wi-Fi failures must never interrupt SD recording. Active wake behavior and radio sharing require Integration Manager confirmation.

Sources: [Estes product](https://estesrockets.com/products/so-long); [official instruction PDF](https://estesrockets.com/cdn/shop/files/Estes-So-Long-Instructions.pdf?v=1722444368); [V17 readiness](../V17_FLIGHT_READINESS.md); [V14 physical wiring](../V14_WIRING_SOLDER.md); [hardware coordination issue #59](https://github.com/aidenjantzi1-ctrl/GREEN-ION/issues/59).

## B. Measurements required BEFORE designing or buying a sized sled

Record measurements to 0.5 mm if possible; do not force a caliper or deform the tube.

| Measurement / photo | How / why |
| --- | --- |
| Payload tube inner diameter | Caliper front/middle/rear, two directions at each; note ovality, seams, glue beads and tape |
| Clear usable axial length | Measure with nose fully seated, aft coupler fully fitted, retaining pin, shock cord and recovery path included |
| Nose/coupler/retention interference | Measure insertion depths, exposed removable length, access/attachment surfaces |
| Component dimensions **including plugs and wires** | Feather with USB-C/JST, SD reader/card, BMP388, IMU, GPS patch, RFM95 breakout, antennas/coax and any carrier |
| Battery actual dimensions/condition | 350 mAh cell length/width/thickness/weight, label, lead bend radius, pack swelling or damage |
| Electrical assemblies | Photos front/back of PCBs, radio RF pad or connector, damaged antennas, solder, JST, chosen breakout versions |
| Flight masses and CG | Empty upper, booster, entire assembled rocket; electronics-sled mass; flight-ready CG for **each chosen motor set** with recovery packed |
| Recovery and staging | Packed streamers/wadding/bundle size; actual free-slide clearance, full-stage coupling engagement, separation paths, motor retention |
| Chamber/port geometry | Actual isolated barometer air volume, available straight smooth payload wall for external static ports, barriers from ejection gases |
| Service needs | How SD card, USB, battery disconnect, antenna and button can be reached when nose is removed |

**Go/no-go for sled geometry:** no dimensioned CAD, drilling or permanent PCB mounting until actual free ID/length, board envelopes, chamber layout and recovery clearance are entered and dry-fit verified. If boards cannot fit without crushing cables or reducing recovery clearance, **stop** and use a larger payload airframe or redesign components, not an unsafe forced fit.

## C. Proposed removable axial layout — preliminary, NOT dimensioned

Nose end → (1) unobstructed PA1010D GPS patch facing suitable sky direction → (2) compact I2C sensor carrier with BMP388 vented to static chamber, IMU rigidly fixed → (3) Feather ESP32 V2 with USB-C service face reachable → (4) flat 350 mAh LiPo in insulated, lightly padded positively retained pocket → (5) microSD breakout oriented for card access + RFM95 radio with mechanically anchored frequency-matched antenna routed axially clear of PCB/battery → aft coupler. **Reorder if actual antenna, CG, access and measured geometry favor another arrangement.** Do not obstruct GPS patch with LiPo or metal; do not route antenna through recovery or stage joint. This is a conceptual sequence, not evidence of a workable fit.

**Structure:** one measured nonconductive narrow backbone (e.g., lightweight PETG 3D print or G10/FR4 strip, finished without abrasive edges); fixed standoffs/PCB holes with nylon hardware; mechanical end stop + captured retaining screw/pin arrangement that does not weaken the tube, snag streamers, block ejection or puncture LiPo. No unsupported tape-only/hot-glue-only mounts. Secure the radio antenna/feed and USB/SD cable ends separately; no connector carries structural load. Include anti-rotation and no bare conductive fastener contacting PCB pads or battery. Avoid floating components and long wire loops.

**LiPo:** preserve the existing **3.7 V 350 mAh** only if it is undamaged and meets endurance tests. Use approved Feather battery JST path, a shaped pouch cradle + nonabrasive padding + dedicated strap/clip; no screw/tie against pouch face, sharp bend, squeeze or battery lead load. Heat-shrink or insulating sleeving over all exposed solder. Use a verified 3.3 V power budget and common ground.

**Wake access:** prefer an externally reachable *recessed mechanical pushrod* operating the Feather's onboard **SW38 / GPIO38** with no new electrical wire, **ONLY** after firmware worker/Integration Manager confirms Wi-Fi wake and management-button semantics and whether sleep state can actually wake from SW38. V17 SW38 management/Wi-Fi authorization behavior is not proof of V18 deep-sleep wake. If unsupported, a new switch requires a separate agreed power/sleep and GPIO contract; do not guess or attach to motor circuits.

**Service:** remove the sled to reach SD, Feather USB-C/JST and replace battery; mark orientation, label SD/LoRa leads and leave small strain-relieved service slack. Verify repeated remove/refit does not loosen RF connection or distort ports.

## D. Wiring diagram contract and electrical acceptance

### Rocket harness (V17 baseline — confirm against merged V18 firmware before soldering)

| Device | Power / signal path | Notes |
| --- | --- | --- |
| Feather ESP32 V2 | Battery to built-in JST; ground common | Inspect cell polarity and connector; no bare LiPo splices |
| BMP388 barometer | I2C SDA **GPIO22**, SCL **GPIO20**, switched 3.3 V/GND as required by breakout | Sample opening faces chamber, not foam |
| LSM6DSO32 IMU | Same I2C SDA22/SCL20 + power/GND | Rigid orientation labeled on sled |
| PA1010D GPS | Same I2C SDA22/SCL20 + appropriate breakout power/GND | Clear patch view; test in final position |
| microSD breakout | SPI SCK **GPIO5**, MOSI **GPIO19**, MISO **GPIO21**, **CS GPIO33**; 3.3 V compatible | Socket accessible; positively retained card, no pin conflicts |
| Adafruit RFM95W rocket breakout | Shared SPI 5/19/21; **CS GPIO27**, **RST GPIO15**, **DIO0 GPIO32**; appropriate supply + common ground | Verify breakout and V18 radio contract; matched antenna before TX |
| Onboard battery sensing | **GPIO35** existing Feather monitor | Do not add external monitor without design review |
| Switched STEMMA QT rail | **GPIO2** existing V17 enable | Validate combined load and sensor power-up |
| Optional buzzer | **GPIO14**, suitable transistor driver only | Omit unless needed; do not drive buzzer from GPIO directly |
| Physical Wi-Fi wake | **SW38/GPIO38** mechanical access proposal | **BLOCKED:** V18 behavior contract required |

Prefer short manufacturer STEMMA QT cables (not loose twisted leads), a mini secure hub only if actual dimensions allow, and compact soldered SPI harness on perfboard with mechanically restrained cable exit. Check shared SPI deselection under simultaneous 50 Hz write + radio activity. No 5 V I/O on Feather pins. Do not bridge regulator/power rails without electrical review.

### Ground station (separate from rocket)
Arduino Uno R3 via USB to Windows Chrome/Edge Web Serial + **regulated/5 V level-shifted Adafruit RFM95 breakout**:
- Uno **5 V→VIN** only on that verified regulated Adafruit breakout; **GND→GND**.
- **D13→SCK, D11→MOSI, D12→MISO, D4→CS, D2→RST, D3→DIO0**.
- Bare SX127x 3.3 V radios must NOT receive Uno 5 V logic. Attach both verified antennas before any RF TX; Uno remains receive-only in current architecture.
- Confirm both ends use identical channel, bandwidth, SF, coding rate, frame contract and legal/club frequency coordination; V17 provisional center frequency **920.5 MHz** is not final field approval.

**Wiring drawing to be completed after measurements:** (a) top-down sled PCB/card positions with dimensions and cable exits; (b) side-section tube + shoulder + coupler + static chamber + streamer path; (c) logical power/I2C/shared SPI schematic with ground, CS, RF feed, JST, wake access; (d) ground receiver pinout; (e) harness labels, wire length, connector protection, SD/USB service clearance. Record actual photos of built harness with test evidence.

## E. ROCstock failure corrections

| Problem | Root cause to check, NOT assumed | Proposed correction and evidence |
| --- | --- | --- |
| Broken LoRa antennas | Missing/bent/cracked radiator, torn solder pad or connector, damaged coax | Power off; magnified photos of both radios/RF feeds, identify 915 MHz compatible antenna type and 50 Ω connection; replace damaged RF hardware; **no transmit until verified matched antennas on both ends** |
| Failed radio | SPI CS/RST/DIO0, power, antenna, board damage, mismatched frequency/settings, packet protocol | Passive chip diagnostic first (SX127x RegVersion 0x12 expected), verify wiring, then 3 short packets with antennas and receiver log, then safe controlled range/packet freshness test |
| Battery drain | Wi-Fi duty, GPS acquisition, TX current, SD writes, bad cell or resets | Measure current in boot/armed/50 Hz recording/LoRa/GPS/Wi-Fi-off/recovery/wake, voltage at peak, brownout log; don't increase battery size until 350 mAh measured |
| Missing pressure ports | Chamber air sealed, leakage into recovery/ejection section or airflow disturbance | Design dedicated external static chamber with vent holes; no tape over BMP388; qualified pressure response test |
| Altitude distorted | Sealed volume, sensor blockage, calibration drift, vent drag or heat | Compare stationary baseline and measured elevation changes; log repeatability and pressure lag; do not treat old unported readings as reliable absolute altitude |
| Tape-mounted electronics | Shifted mass, strain and unreliable connections | Positive sled retention, standoffs, battery cradle, insulated joints, strain relief and hand-rotation/tap test |

**Static ports:** after measuring actual **sealed barometer chamber effective volume**, consult altimeter-maker static-port guidance to size *several small deburred holes distributed circumferentially* in a straight, smooth tube wall outside the nose/coupler disturbance; keep ejection gas and wadding out. Do not drill a standard 38/54 mm bay recipe into this tiny 31 mm **outside** tube. Review hole locations for structural integrity and validate real step response. Only after that freeze size/count/position and obtain mentor/RSO review.

References: [Altus Metrum](https://altusmetrum.org/AltOS/doc/micropeak.html), [PerfectFlite altimeter manual](https://www.perfectflite.com/Downloads/MAWDManual.pdf), [ROC tracking coordination](https://rocstock.org/rocket-tracking/), [V17 readiness](../V17_FLIGHT_READINESS.md).

## F. Shopping list — buy only after stocktaking/dimension checks

| Item / specification | Quantity | Budgetary cost (USD; not verified live price) | Buy gate |
| --- | --- | --- | --- |
| Lightweight custom nonconductive sled material + nylon M2/M2.5 hardware | 1 set | $5–20 | Measured clear ID/length and PCB holes |
| Small PETG or G10/FR4 retaining parts / soft LiPo pad + nonabrasive battery strap | 1 | $3–12 | Battery dimensions and restraint proof |
| Heat-shrink, thin insulated wire, short STEMMA QT leads, wire anchors | 1 lot | $8–20 | Exact wiring inventory |
| Frequency-matched **915 MHz-region** flexible whip or correct connectorized antenna per RFM95 board | 1–2 | $5–20 each | Determine connector/antenna failure; TX remains disabled until verified |
| Replacement RFM95W Adafruit breakout **only if damaged** | 0–1 | $15–30 | Passive SPI and board physical inspection |
| Replacement compatible 3.7 V 350 mAh LiPo **only if worn/damaged** | 0–1 | $7–15 | Match JST polarity, size, protection and Feather charge constraints |
| SD reader/card or low-profile breakout replacement **only if needed** | 0–1 | $5–15 | Confirm exact dimensions, 3.3 V logic and high-rate SD integrity |
| Calipers + small gram scale (borrow if possible) | optional | $0 borrowed / $15–35 each | Essential measurements |
| Multimeter (borrow) and USB current meter (optional) | 1 | $0 borrowed / $10–25 | Battery/current evidence |
| Tube/drill/deburr supplies | as needed | $0–15 | **After** static-port size review |

**Prices are only planning allowances, not seller quotes; verify local/current prices before ordering.** Do not purchase new Feather, GPS or sensor boards if the existing ones pass electrical and fit tests. No spare power-on RF tests with an antenna disconnected.

## G. Beginner-friendly assembly sequence (MOTORS REMOVED)

1. Photograph and inventory all boards, plugs, battery label and existing damage; identify exact microSD breakout and 915 MHz antenna interfaces.
2. Measure tube ID/usable length, coupler/nose intrusion, recovery bundle, board envelopes including cables. Record all values in section B. **STOP if clearance uncertain.**
3. Dry-fit cardboard/printed templates; mark GPS sightline, barometer chamber, SD/USB service and antenna route. Test stage and streamer free travel without electronics.
4. With USB/LiPo unplugged, inspect radio/coax, solder pads and antenna; replace questionable parts.
5. Test Feather, BMP388, IMU, GPS, SD individually on bench; then integrate shared I2C and SPI according to confirmed V18 pin map.
6. Measure no-short 3.3 V/GND, verify positive/ground pinout, then power with USB bench supply; connect inspected LiPo through approved JST only.
7. Fabricate insulated axial sled from the **measured** dimensions. Mount electronics on secure standoffs; create dedicated padded battery cradle with positive capture.
8. Secure and sleeve solder joints/short STEMMA leads, SD socket and RF connector; route antenna away from battery/metal/streamers; add strain relief and labels.
9. Confirm Integration Manager's physical wake GPIO contract; test button mechanically and electrically **before** adding permanent rocket access hole.
10. Establish static chamber and mentor-approved ports **only after** chamber-volume sizing review. Avoid exposing barometer to ejection gases.
11. Run all home tests below with sled removed, then installed in the empty rocket tube. Store test logs and photos.
12. Fit/pack nose, streamers, shock cord and coupler. Prove zero interference, do not alter stage coupling or motor handling outside Estes instructions.
13. Weigh and measure CG **with each exact intended motor combination** and packed recovery; simulate full stack and poststage upper stage, obtain experienced mentor/RSO review.
14. Only after evidence closes every gate may the responsible RSO inspect the real vehicle; approval remains external.

## H. Motor-free home tests: explicit acceptance and failure actions

**General:** keep all motors and igniters physically removed. Use a stable bench, safe LiPo practices; do not test by throwing, launching or accelerating a powered rocket.

| Check | PASS evidence | FAIL action |
| --- | --- | --- |
| Mechanical | Sled slides in/out without force; retained against hand motion; no pinched wires, loose boards or LiPo compression | Redesign mounting/strain relief, re-test |
| Power polarity | No shorts, correct Feather/regulator pins, clean USB/LiPo transitions | Disconnect immediately, inspect with mentor |
| BMP388 calibration | Stable stationary baseline after settling; repeat calibration agrees within chosen measured tolerance | Check vent, sensor and temperature, retry; record numeric tolerance |
| IMU | All axes respond correctly to slow hand rotations, correct static magnitude and labeled orientation | Check I2C/address, mount rigidly, rerun |
| SD 50 Hz | Full Home Test synthetic file size/header/readback pass; repeated actual BIN recording decodes without gaps/errors; no data loss from Wi-Fi or LoRa disconnect | Fix SD power/socket/SPI/card, verify integrity; **NO GO** |
| Battery | Measured complete mission soak with >= 2× planned pad+flight+recovery duration and voltage margin; no brownouts/resets | Reduce current load, improve supply, replace damaged cell; repeat |
| GPS outdoors | Fix in final sled orientation; coordinates plausible, freshness and acquisition times logged | Move away from blockage, inspect GPS power/cable; recovery GPS NOT QUALIFIED |
| Radio passive | RFM95 chip/version/init response on rocket and Uno; no hot spots/wiring fault | Inspect power, SPI, CS/RST/DIO0; replace faulty board |
| Antennas + RF | Physical check of both RF paths, then 3 transmitted bench packets received/validated; packet age and receiver USB reconnect work; range/packet-loss measured in controlled legal environment | **No TX** if antenna unknown; otherwise fix settings/cable/receiver; recovery LoRa NOT QUALIFIED |
| Wake button | From actual defined V18 sleeping/landed state, recessed button reliably enables expected Wi-Fi access; no unintended reset during recording; >=10 presses | Escalate SW38/GPIO contract; do not cut access hole |
| Multi-flight | Three complete *simulated/bench* prepare-arm-record-finalize cycles, distinct preserved BINs, no overwrite, no storage leaks, recover after controlled power-cycle when safe | Review recorder/storage and escalate software issue; **NO GO** |
| Pressure ports | Repeatable reference pressure/elevation response and minimal observed lag/no ejection-chamber leakage | Resize/relocate ports after mentor review; do not trust altitude |
| Recovery tracking | With live RX and safe antenna, receiver shows latest plausible coordinates, freshness, stale/loss indication, re-acquisition after unplug | Fix serial framing/RF/GPS; label recovery unqualified |
| Recovery and stages | Streamers/wadding free slide, no electronics snag, stage fit and motor retention consistent with Estes manual | Redesign sled or recovery packing; **NO GO** |
| Stability | Actual loaded mass/CG per motor pair; validated full-stack + upper-stage CP/CG model and RSO-approved margin | Adjust payload/airframe/motor, re-run, **NO GO** |

**Do not replace measurements with software simulations.** Save dated photos of finished assemblies, recorded numeric results, battery current graph/log, SD BIN sample and hash, GPS fix and RF packet logs, static-port test, signed mentor/RSO checklist.

## I. Stop-ship blockers and Integration Manager action requests

**OPEN BLOCKERS:** (1) no actual tube inside/usable axial size; (2) board+lead and battery dimensions unmeasured; (3) damaged radio/antenna unresolved and RF unqualified; (4) actual wake/sleep contract unconfirmed; (5) no static chamber/port calculation/test; (6) battery load/endurance unknown; (7) no physically secured removable sled; (8) no actual flight-ready CG/stability model for chosen motor configuration; (9) recovery/stage clearance not demonstrated; (10) no physical full-home/multi-flight/recovery test evidence; (11) no launch club frequency coordination and RSO inspection.

**Integration Manager:** review this branch/doc together with [issue #59](https://github.com/aidenjantzi1-ctrl/GREEN-ION/issues/59), reconcile merged V18 pin map, SW38 button wake contract, 50 Hz priority and LoRa packet/frequency settings; merge **documentation only** after review. Never mark the vehicle or V18 hardware flight-ready from this proposed document alone.
