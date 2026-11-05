/*
  GREEN ION V12 Arduino Uno ground receiver.
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

RH_RF95 radio(PIN_CS, PIN_DIO0);
uint32_t validPackets = 0;
uint32_t invalidPackets = 0;
uint32_t lastPacketMs = 0;
bool haveLastPosition = false;
int32_t lastLatitudeE7 = 0;
int32_t lastLongitudeE7 = 0;
uint32_t lastPositionReceivedMs = 0;

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

static void printTelemetry(const green_ion::TelemetryPacket &packet, int16_t rssi) {
  const bool current = green_ion::hasCurrentPosition(packet);
  const bool bench = green_ion::isBenchRadioPacket(packet);
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
  Serial.print(F(",\"valid_packets\":"));
  Serial.print(validPackets);
  Serial.print(F(",\"invalid_packets\":"));
  Serial.print(invalidPackets);
  Serial.print(F(",\"latitude\":"));
  if (green_ion::hasPosition(packet)) printScaledDegrees(packet.latitudeE7); else Serial.print(F("null"));
  Serial.print(F(",\"longitude\":"));
  if (green_ion::hasPosition(packet)) printScaledDegrees(packet.longitudeE7); else Serial.print(F("null"));
  Serial.print(F(",\"last_position_age_ms\":"));
  if (haveLastPosition) Serial.print(millis() - lastPositionReceivedMs); else Serial.print(F("null"));
  Serial.println(F("}"));
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);
  delay(10);
  digitalWrite(PIN_RST, HIGH);
  delay(10);

  if (!radio.init() || !radio.setFrequency(green_ion::radio_config::kFrequencyMhz)) {
    Serial.println(F("{\"type\":\"receiver_status\",\"ok\":false,\"error\":\"radio_init\"}"));
    return;
  }
  radio.setTxPower(green_ion::radio_config::kTxPowerDbm, false);
  radio.setSpreadingFactor(green_ion::radio_config::kSpreadingFactor);
  radio.setSignalBandwidth(green_ion::radio_config::kSignalBandwidthHz);
  radio.setCodingRate4(green_ion::radio_config::kCodingRateDenominator);
  Serial.print(F("{\"type\":\"receiver_status\",\"ok\":true,\"frequency_mhz\":"));
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

void loop() {
  if (!radio.available()) return;
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
  printTelemetry(packet, radio.lastRssi());
}

