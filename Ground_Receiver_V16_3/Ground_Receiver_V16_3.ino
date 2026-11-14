/*
  GREEN ION V17.0 Arduino Uno ground receiver.
  Wiring is for the Adafruit RFM95W breakout: CS D4, RST D2, DIO0 D3,
  hardware SPI D13/D11/D12. Output is one JSON object per valid packet.
*/

#include <SPI.h>
#include <RH_RF95.h>
#include <GreenIonProtocol.h>
#include <GreenIonRadioConfig.h>

static const uint8_t PIN_CS = 4;
static const uint8_t PIN_RST = 2;
static const uint8_t PIN_DIO0 = 3;
static const uint32_t SERIAL_BAUD = 115200;
static const char RECEIVER_FIRMWARE_VERSION[] = "17.0.1";

RH_RF95 radio(PIN_CS, PIN_DIO0);
uint32_t validPackets = 0;
uint32_t invalidPackets = 0;
uint32_t lastPacketMs = 0;
uint32_t estimatedMissingPackets = 0;
uint32_t lastSequence = 0;
uint32_t lastUptimeMs = 0;
uint32_t duplicatePackets = 0, outOfOrderPackets = 0, senderRestarts = 0;
uint32_t receivedBenchPackets = 0;
uint32_t lastStatusMs = 0;
bool radioReady = false;
uint8_t radioChipVersion = 0;
bool haveLastSequence = false;
bool humanReadable = false;
char serialCommand[20] = {};
uint8_t serialCommandLength = 0;
bool haveLastPosition = false;
int32_t lastLatitudeE7 = 0;
int32_t lastLongitudeE7 = 0;
uint32_t lastPositionReceivedMs = 0;

static uint8_t readRadioVersionRegister() {
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  SPI.transfer(0x42 & 0x7f);
  const uint8_t version = SPI.transfer(0);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
  return version;
}

static void printReceiverDiagnostic() {
  Serial.print(F("{\"type\":\"receiver_diagnostic\",\"receiver_firmware\":\""));
  Serial.print(RECEIVER_FIRMWARE_VERSION);
  Serial.print(F("\",\"radio_ready\":"));
  Serial.print(radioReady ? F("true") : F("false"));
  Serial.print(F(",\"chip_version\":"));
  Serial.print(radioChipVersion);
  Serial.print(F(",\"expected_chip_version\":18,\"pins\":{\"cs\":4,\"rst\":2,\"dio0\":3,\"sck\":13,\"mosi\":11,\"miso\":12}"));
  Serial.print(F(",\"settings\":{\"frequency_mhz\":"));
  Serial.print(green_ion::radio_config::kFrequencyMhz,1);
  Serial.print(F(",\"bandwidth_hz\":"));
  Serial.print(green_ion::radio_config::kSignalBandwidthHz);
  Serial.print(F(",\"spreading_factor\":"));
  Serial.print(green_ion::radio_config::kSpreadingFactor);
  Serial.print(F(",\"coding_rate\":\"4/"));
  Serial.print(green_ion::radio_config::kCodingRateDenominator);
  Serial.print(F("\"},\"receive_only\":true,\"antenna_detectable\":false,\"probable_problem\":\""));
  if (radioChipVersion == 0x12 && radioReady)
    Serial.print(F("none detected in software; compare Feather settings and antennas if packets are absent"));
  else if (radioChipVersion == 0x00)
    Serial.print(F("0x00 chip ID: check 3.3V/GND, MISO, SCK and CS D4"));
  else if (radioChipVersion == 0xff)
    Serial.print(F("0xFF chip ID: MISO may float high or CS D4/power may be wrong"));
  else if (radioChipVersion != 0x12)
    Serial.print(F("unexpected SX127x chip ID: check radio model and SPI wiring"));
  else
    Serial.print(F("chip ID is valid but init failed: check RST D2, DIO0 D3 and matching radio settings"));
  Serial.println(F("\"}"));
}

static void printScaledDegrees(int32_t value) {
  if (value < 0) {
    Serial.print('-');
    value = -value;
  }
  Serial.print(value / 10000000L);
  Serial.print('.');
  long fraction = value % 10000000L;
  for (long divisor = 1000000L; divisor > fraction && divisor > 1; divisor /= 10) {
    Serial.print('0');
  }
  Serial.print(fraction);
}

static void printTelemetry(const green_ion::TelemetryPacket &packet, int16_t rssi, int8_t snr) {
  const bool current = green_ion::hasCurrentPosition(packet);
  const bool bench = green_ion::isBenchRadioPacket(packet);
  if (haveLastSequence && packet.uptimeMs < lastUptimeMs &&
      lastUptimeMs - packet.uptimeMs >= 1000UL &&
      packet.sequence <= 5 && packet.sequence < lastSequence) {
    // Corroborated sender reboot: low new sequence AND substantive uptime drop.
    ++senderRestarts;
    haveLastSequence = false;
  }
  bool advanceSequence = true;
  if (haveLastSequence) {
    const uint32_t gap = packet.sequence - lastSequence;
    if (gap == 0) { ++duplicatePackets; advanceSequence = false; }
    else if (gap > 1 && gap < 1000000UL) estimatedMissingPackets += gap - 1;
    else if (gap >= 1000000UL) {
      ++outOfOrderPackets;
      advanceSequence = false;
    }
  }
  // Duplicates and late packets must not move the sequence high-watermark
  // backwards or create imaginary missing packets on the following frame.
  if (advanceSequence) {
    lastSequence = packet.sequence;
    lastUptimeMs = packet.uptimeMs;
    haveLastSequence = true;
  }
  if (bench) ++receivedBenchPackets;
  if (green_ion::hasPosition(packet)) {
    haveLastPosition = true;
    lastLatitudeE7 = packet.latitudeE7;
    lastLongitudeE7 = packet.longitudeE7;
    lastPositionReceivedMs = millis();
  }

  Serial.print(bench ? F("{\"type\":\"radio_bench\",\"protocol\":") :
                       F("{\"type\":\"telemetry\",\"protocol\":"));
  Serial.print(packet.version);
  Serial.print(F(",\"sequence\":"));
  Serial.print(packet.sequence);
  Serial.print(F(",\"uptime_ms\":"));
  Serial.print(packet.uptimeMs);
  Serial.print(F(",\"packet_age_ms\":0,\"rssi_dbm\":"));
  Serial.print(rssi);
  Serial.print(F(",\"snr_db\":"));
  Serial.print(snr);
  Serial.print(F(",\"altitude_m\":"));
  Serial.print(packet.altitudeCm / 100.0f, 2);
  Serial.print(F(",\"pressure_pa\":"));
  Serial.print(packet.pressurePa);
  Serial.print(F(",\"gps_altitude_m\":"));
  Serial.print(packet.gpsAltitudeCm / 100.0f, 2);
  Serial.print(F(",\"acceleration_g\":"));
  Serial.print(packet.accelerationMg / 1000.0f, 3);
  Serial.print(F(",\"acceleration_x_g\":"));
  Serial.print(packet.accelerationXMg / 1000.0f, 3);
  Serial.print(F(",\"acceleration_y_g\":"));
  Serial.print(packet.accelerationYMg / 1000.0f, 3);
  Serial.print(F(",\"acceleration_z_g\":"));
  Serial.print(packet.accelerationZMg / 1000.0f, 3);
  Serial.print(F(",\"rotation_x_dps\":"));
  Serial.print(packet.rotationXDps10 / 10.0f, 1);
  Serial.print(F(",\"rotation_y_dps\":"));
  Serial.print(packet.rotationYDps10 / 10.0f, 1);
  Serial.print(F(",\"rotation_z_dps\":"));
  Serial.print(packet.rotationZDps10 / 10.0f, 1);
  Serial.print(F(",\"speed_mps\":"));
  Serial.print(packet.speedCms / 100.0f, 2);
  Serial.print(F(",\"battery_v\":"));
  Serial.print(packet.batteryMv / 1000.0f, 3);
  Serial.print(F(",\"gps_fix_age_ms\":"));
  Serial.print(packet.gpsFixAgeMs);
  Serial.print(F(",\"gps_status\":\""));
  Serial.print(current ? F("live") : (green_ion::hasPosition(packet) ? F("stale") : F("missing")));
  Serial.print(F("\",\"satellites\":"));
  Serial.print(packet.satellites);
  Serial.print(F(",\"flight_state\":"));
  Serial.print(packet.flightState);
  Serial.print(F(",\"flags\":"));
  Serial.print(packet.flags);
  Serial.print(F(",\"health\":"));
  Serial.print(packet.health);
  Serial.print(F(",\"estimated_missing_packets\":"));
  Serial.print(estimatedMissingPackets);
  Serial.print(F(",\"valid_packets\":"));
  Serial.print(validPackets);
  Serial.print(F(",\"invalid_packets\":"));
  Serial.print(invalidPackets);
  Serial.print(F(",\"bench_received\":"));
  Serial.print(receivedBenchPackets);
  Serial.print(F(",\"sender_restarts\":"));
  Serial.print(senderRestarts);
  Serial.print(F(",\"duplicates\":"));
  Serial.print(duplicatePackets);
  Serial.print(F(",\"out_of_order\":"));
  Serial.print(outOfOrderPackets);
  Serial.print(F(",\"latitude\":"));
  if (green_ion::hasPosition(packet)) printScaledDegrees(packet.latitudeE7); else Serial.print(F("null"));
  Serial.print(F(",\"longitude\":"));
  if (green_ion::hasPosition(packet)) printScaledDegrees(packet.longitudeE7); else Serial.print(F("null"));
  Serial.print(F(",\"last_position_age_ms\":"));
  if (haveLastPosition) Serial.print(millis() - lastPositionReceivedMs); else Serial.print(F("null"));
  Serial.println(F("}"));
  // Human-friendly optional summary; JSON always remains available to Python.
  if (humanReadable) {
    Serial.print(F("[RADIO] #"));
    Serial.print(packet.sequence);
    Serial.print(F(" | Alt: "));
    Serial.print(packet.altitudeCm / 100.0f, 1);
    Serial.print(F(" m | Battery: "));
    Serial.print(packet.batteryMv / 1000.0f, 2);
    Serial.print(F(" V | GPS: "));
    Serial.print(current ? F("LIVE") : (green_ion::hasPosition(packet) ? F("STALE") : F("NONE")));
    Serial.print(F(" | RSSI: "));
    Serial.print(rssi);
    Serial.print(F(" dBm | Missed est.: "));
    Serial.println(estimatedMissingPackets);
  }
}

static void handleSerialCommands() {
  while (Serial.available()) {
    const char value = static_cast<char>(Serial.read());
    if (value == '\r') continue;
    if (value == '\n') {
      serialCommand[serialCommandLength] = '\0';
      if (!strcmp(serialCommand, "human on")) {
        humanReadable = true;
        Serial.println(F("[HELP] Human summaries ON. JSON telemetry continues for Python."));
      } else if (!strcmp(serialCommand, "human off")) {
        humanReadable = false;
        Serial.println(F("[HELP] Human summaries OFF. JSON telemetry continues."));
      } else if (!strcmp(serialCommand, "diag")) {
        radioChipVersion = readRadioVersionRegister();
        printReceiverDiagnostic();
      } else if (!strcmp(serialCommand, "status")) {
        Serial.print(F("[STATUS] Receiver valid: "));
        Serial.print(validPackets);
        Serial.print(F(" | CRC/rejected: "));
        Serial.print(invalidPackets);
        Serial.print(F(" | Estimated missed: "));
        Serial.println(estimatedMissingPackets);
      } else if (serialCommandLength) {
        Serial.println(F("[HELP] Commands: human on | human off | status | diag"));
      }
      serialCommandLength = 0;
    } else if (serialCommandLength + 1 < sizeof(serialCommand)) {
      serialCommand[serialCommandLength++] = value;
    } else {
      serialCommandLength = 0;
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  SPI.begin();
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);
  delay(10);
  digitalWrite(PIN_RST, HIGH);
  delay(10);
  radioChipVersion = readRadioVersionRegister();

  if (!radio.init() || !radio.setFrequency(green_ion::radio_config::kFrequencyMhz)) {
    Serial.print(F("{\"type\":\"receiver_status\",\"receiver_firmware\":\""));
    Serial.print(RECEIVER_FIRMWARE_VERSION);
    Serial.print(F("\",\"ok\":false,\"error\":\"radio_init\",\"chip_version\":"));
    Serial.print(radioChipVersion);
    Serial.println(F("}"));
    printReceiverDiagnostic();
    return;
  }
  radio.setTxPower(green_ion::radio_config::kTxPowerDbm, false);
  radio.setSpreadingFactor(green_ion::radio_config::kSpreadingFactor);
  radio.setSignalBandwidth(green_ion::radio_config::kSignalBandwidthHz);
  radio.setCodingRate4(green_ion::radio_config::kCodingRateDenominator);
  radioReady = true;
  Serial.print(F("{\"type\":\"receiver_status\",\"ok\":true,\"receiver_firmware\":\""));
  Serial.print(RECEIVER_FIRMWARE_VERSION);
  Serial.print(F("\",\"protocol\":"));
  Serial.print(green_ion::kProtocolVersion);
  Serial.print(F(",\"frequency_mhz\":"));
  Serial.print(green_ion::radio_config::kFrequencyMhz, 1);
  Serial.print(F(",\"bandwidth_hz\":"));
  Serial.print(green_ion::radio_config::kSignalBandwidthHz);
  Serial.print(F(",\"spreading_factor\":"));
  Serial.print(green_ion::radio_config::kSpreadingFactor);
  Serial.print(F(",\"coding_rate\":\"4/"));
  Serial.print(green_ion::radio_config::kCodingRateDenominator);
  Serial.print(F("\",\"tx_power_dbm\":"));
  Serial.print(green_ion::radio_config::kTxPowerDbm);
  Serial.println(F(",\"antenna_required\":true}"));
}


static void printReceiverHeartbeat(uint32_t nowMs) {
  if (nowMs - lastStatusMs < 5000UL) return;
  lastStatusMs = nowMs;
  Serial.print(F("{\"type\":\"receiver_status\",\"receiver_firmware\":\""));
  Serial.print(RECEIVER_FIRMWARE_VERSION);
  Serial.print(F("\",\"ok\":"));
  Serial.print(radioReady ? F("true") : F("false"));
  Serial.print(F(",\"protocol\":")); Serial.print(green_ion::kProtocolVersion);
  Serial.print(F(",\"valid_packets\":")); Serial.print(validPackets);
  Serial.print(F(",\"invalid_packets\":")); Serial.print(invalidPackets);
  Serial.print(F(",\"estimated_missing_packets\":")); Serial.print(estimatedMissingPackets);
  Serial.print(F(",\"bench_received\":")); Serial.print(receivedBenchPackets);
  Serial.print(F(",\"last_packet_age_ms\":"));
  if (validPackets) Serial.print(nowMs - lastPacketMs); else Serial.print(F("null"));
  Serial.print(F(",\"chip_version\":")); Serial.print(radioChipVersion);
  Serial.println(F("}"));
}

void loop() {
  handleSerialCommands();
  printReceiverHeartbeat(millis());
  if (!radioReady || !radio.available()) return;
  uint8_t buffer[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t length = sizeof(buffer);
  if (!radio.recv(buffer, &length)) {
    ++invalidPackets;
    return;
  }
  green_ion::ValidationResult result = green_ion::validate(buffer, length);
  if (result != green_ion::VALID) {
    ++invalidPackets;
    Serial.print(F("{\"type\":\"receiver_error\",\"code\":"));
    Serial.print(static_cast<uint8_t>(result));
    Serial.print(F(",\"invalid_packets\":"));
    Serial.print(invalidPackets);
    Serial.println(F("}"));
    return;
  }
  ++validPackets;
  lastPacketMs = millis();
  green_ion::TelemetryPacket packet;
  memcpy(&packet, buffer, sizeof(packet));
  printTelemetry(packet, radio.lastRssi(), radio.lastSNR());
}

