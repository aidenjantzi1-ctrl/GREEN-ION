# GREEN ION V17.0 — current Arduino Uno LoRa receiver

**Current software generation: V17.0 (`17.0.1`)**. Legacy sketch directory name `Ground_Receiver_V16_3/` is retained for Arduino build and compatibility with the source tree.

- Active sketch: `Ground_Receiver_V16_3.ino`, reporting receiver firmware `17.0.1`.
- Receive-only RFM95W ground telemetry, still protocol v2 at **70 bytes**.
- Serial command `diag` reports SX127x chip ID, receiver pin map and matching radio settings; it does not transmit or claim antenna detection.
- Official release HEX name: `Ground_Receiver_V17.ino.hex`.
- [V17 release](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/tag/v17.0.1) · [Update guide](../docs/V17_INSTALL_UPDATE.md).

Confirm reception, power, antenna and frequency coordination on actual hardware; compilation alone is not a range or flight qualification.
