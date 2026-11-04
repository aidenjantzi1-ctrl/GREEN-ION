#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "GreenIonMetadata.h"

using namespace green_ion::metadata;

int main() {
  const FlightStart start = {
    "/V12D007.BIN", 50, 101325.0f, 0.125f, 4010, 150,
  };
  const char expected[] =
      "GREEN-ION-METADATA-1\n"
      "status=recording\n"
      "file=V12D007.BIN\n"
      "rkt4_version=2\n"
      "sample_rate_hz=50\n"
      "baseline_pressure_pa=101325.0\n"
      "calibration_noise_m=0.125\n"
      "start_battery_mv=4010\n"
      "prebuffer_records=150\n"
      "checkpoint_format=GIC2\n";
  char output[320];
  size_t written = 0;
  assert(encodeFlightStart(output, sizeof(output), start, written) == METADATA_VALID);
  assert(written == strlen(expected));
  assert(strcmp(output, expected) == 0);

  char shortBuffer[32];
  assert(encodeFlightStart(shortBuffer, sizeof(shortBuffer), start, written) ==
         METADATA_BUFFER_TOO_SMALL);
  assert(written == 0);
  FlightStart badName = start;
  badName.logFilename = "/OTHER.BIN";
  assert(encodeFlightStart(output, sizeof(output), badName, written) ==
         METADATA_INVALID_FILENAME);
  FlightStart badRate = start;
  badRate.sampleRateHz = 20;
  assert(encodeFlightStart(output, sizeof(output), badRate, written) ==
         METADATA_INVALID_VALUES);
  FlightStart badNoise = start;
  badNoise.calibrationNoiseM = NAN;
  assert(encodeFlightStart(output, sizeof(output), badNoise, written) ==
         METADATA_INVALID_VALUES);

  const FlightComplete complete = {
    "/V12D007.BIN", 45200, 32 + 200 * 16, 200, 3, 3920, 125.5f,
  };
  const char expectedComplete[] =
      "GREEN-ION-COMPLETION-1\n"
      "status=complete\n"
      "finish_reason=automatic_landing\n"
      "file=V12D007.BIN\n"
      "duration_ms=45200\n"
      "file_bytes=3232\n"
      "complete_records=200\n"
      "missed_sample_periods=3\n"
      "end_battery_mv=3920\n"
      "peak_altitude_m=125.50\n";
  assert(encodeFlightComplete(output, sizeof(output), complete, written) ==
         METADATA_VALID);
  assert(written == strlen(expectedComplete));
  assert(strcmp(output, expectedComplete) == 0);
  FlightComplete torn = complete;
  torn.fileBytes += 1;
  assert(encodeFlightComplete(output, sizeof(output), torn, written) ==
         METADATA_INVALID_VALUES);

  puts("GREEN ION V12 metadata tests passed");
  return 0;
}
