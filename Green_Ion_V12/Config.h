#pragma once

#include <Arduino.h>

namespace config {

// Feather ESP32 V2 pins. Keep every assignment here so hardware changes do not
// leak into sensor or logging logic.
static const uint8_t PIN_I2C_SDA = 22;
static const uint8_t PIN_I2C_SCL = 20;
static const uint8_t PIN_SPI_SCK = 5;
static const uint8_t PIN_SPI_MOSI = 19;
static const uint8_t PIN_SPI_MISO = 21;
static const uint8_t PIN_SD_CS = 33;
static const uint8_t PIN_LORA_CS = 27;
static const uint8_t PIN_LORA_RST = 15;
static const uint8_t PIN_LORA_DIO0 = 32;
static const uint8_t PIN_BUZZER = 14;  // Drives a transistor, never the buzzer directly.
static const uint8_t PIN_BATTERY = 35;
static const uint8_t PIN_I2C_POWER = 2;

static const uint32_t SERIAL_BAUD = 115200;
static const uint16_t SENSOR_RATE_HZ = 50;
static const uint32_t SENSOR_INTERVAL_US = 1000000UL / SENSOR_RATE_HZ;
static const uint32_t GPS_SERVICE_INTERVAL_MS = 20;
static const uint32_t TELEMETRY_INTERVAL_MS = 1000;
static const uint32_t LOG_FLUSH_INTERVAL_MS = 500;
static const uint32_t GPS_CURRENT_MAX_AGE_MS = 3000;

// Verify frequency, antenna, power, bandwidth, spreading factor, and duty-cycle
// constraints for the launch location before transmitting.
static constexpr float LORA_FREQUENCY_MHZ = 915.0f;
static const int8_t LORA_TX_POWER_DBM = 13;
static const uint8_t LORA_SPREADING_FACTOR = 7;
static const long LORA_SIGNAL_BANDWIDTH_HZ = 125000;
static const uint8_t LORA_CODING_RATE_DENOMINATOR = 5;

static const bool BUZZER_ENABLED = true;
static const uint16_t BUZZER_FREQUENCY_HZ = 2500;

static_assert(PIN_SD_CS != PIN_LORA_CS, "SD and LoRa require separate chip selects");
static_assert(SENSOR_RATE_HZ > 0 && SENSOR_RATE_HZ <= 200, "Unsupported sensor rate");

}  // namespace config

