# GREEN ION V16.2 — Arduino Uno receiver

Target: **Arduino Uno R3 + matching RFM9x 900 MHz receiver**.

V16.2 receiver source is `Past_Versions/V16/Ground_Receiver_V16_2/Ground_Receiver_V16_2.ino`; release HEX is `Ground_Receiver_V16_2.ino.hex`.

The Uno receiver remains **receive-only**. It does not transmit commands to the rocket.

## Update

The simplest supported path is Arduino IDE/Arduino CLI from the release source ZIP:

1. Disconnect the receiver from any launch hardware; use a bench setup.
2. Confirm the RFM9x wiring and antenna are correct.
3. Remember: the Uno uses 5 V logic while the radio is 3.3 V hardware. Use the existing appropriate level/power protection; software cannot verify this wiring.
4. Open `Ground_Receiver_V16_2.ino`.
5. Select Arduino Uno.
6. Upload over USB.
7. Reconnect the Windows ground station.
8. System Health/receiver JSON should report receiver firmware `16.2.0` and protocol v2.

A prebuilt HEX is included for users with a trusted HEX-flashing workflow, but the source upload path is easier to audit.

## Compatibility

V16.2 deliberately keeps the CRC-protected protocol-v2 **70-byte** telemetry packet. Existing v2 field layout and radio airtime stay unchanged. New Feather health bits use spare bits in the existing 16-bit health field; older v2 receivers may ignore unknown bits.
