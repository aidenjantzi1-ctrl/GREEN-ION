#pragma once

#include <Arduino.h>
#include <math.h>

struct SensorState {
  bool barometerOk = false;
  bool imuOk = false;
  bool imuDataValid = false;
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
  float imuTemperatureC = NAN;
  uint16_t batteryMv = 0;

  bool gpsHasPosition = false;
  bool gpsFixCurrent = false;
  uint32_t lastGpsFixMs = 0;
  uint32_t lastGpsSentenceMs = 0;
  uint32_t imuReadFailures = 0;
  uint8_t imuConsecutiveReadFailures = 0;
  uint32_t gpsParseFailures = 0;
  uint32_t gpsNmeaSentences = 0;
  uint32_t gpsParsedSentences = 0;
  uint32_t lastGpsNmeaMs = 0;
  double latitude = 0.0;
  double longitude = 0.0;
  float gpsAltitudeM = NAN;
  float gpsSpeedMps = NAN;
  float gpsCourseDeg = NAN;       // RMC ground-track course, not a compass heading
  float gpsHdop = NAN;            // Horizontal dilution of precision, not meter accuracy
  float gpsGeoidSeparationM = NAN;// GGA geoid/WGS84 separation
  uint8_t gpsFixQuality = 0;     // GGA reported fix quality

  uint8_t satellites = 0;
  uint8_t gpsHour = 0;
  uint8_t gpsMinute = 0;
  uint8_t gpsSecond = 0;
};

