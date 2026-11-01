#pragma once

#include <Arduino.h>
#include <math.h>

struct SensorState {
  bool barometerOk = false;
  bool imuOk = false;
  bool gpsOk = false;
  bool sdOk = false;
  bool radioOk = false;
  bool recording = false;
  bool buzzerActive = false;

  float pressurePa = NAN;
  float temperatureC = NAN;
  float altitudeM = NAN;
  float accelX = NAN;
  float accelY = NAN;
  float accelZ = NAN;
  float accelMagnitude = NAN;
  float gyroX = NAN;
  float gyroY = NAN;
  float gyroZ = NAN;
  uint16_t batteryMv = 0;

  bool gpsHasPosition = false;
  uint32_t lastGpsFixMs = 0;
  double latitude = 0.0;
  double longitude = 0.0;
  float gpsAltitudeM = NAN;
  float gpsSpeedMps = NAN;
  uint8_t satellites = 0;
  uint8_t gpsHour = 0;
  uint8_t gpsMinute = 0;
  uint8_t gpsSecond = 0;
};

