#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "GreenIonCalibration.h"

using namespace green_ion::calibration;

int main() {
  float stable[96];
  for (size_t i = 0; i < 96; ++i)
    stable[i] = 101325.0f + (static_cast<int>(i % 5) - 2) * 0.15f;
  stable[30] = 100000.0f;
  stable[60] = 103000.0f;

  Result result = analyze<96>(stable, 96, 4.0f);
  assert(result.code == CALIBRATION_VALID);
  assert(fabsf(result.baselinePressurePa - 101325.0f) < 0.1f);
  assert(result.noiseAltitudeM < 0.1f);
  assert(fabsf(result.driftPaPerSecond) < 0.1f);

  assert(analyze<96>(stable, 79, 4.0f).code == CALIBRATION_TOO_FEW_SAMPLES);
  assert(analyze<96>(stable, 96, 0.0f).code == CALIBRATION_BAD_DURATION);

  float invalid[96];
  for (size_t i = 0; i < 96; ++i) invalid[i] = stable[i];
  invalid[40] = 20000.0f;
  assert(analyze<96>(invalid, 96, 4.0f).code == CALIBRATION_INVALID_SAMPLE);

  float noisy[96];
  for (size_t i = 0; i < 96; ++i)
    noisy[i] = 101325.0f + (static_cast<int>(i % 2) ? 12.0f : -12.0f);
  assert(analyze<96>(noisy, 96, 4.0f).code == CALIBRATION_TOO_NOISY);

  float drifting[96];
  for (size_t i = 0; i < 96; ++i) drifting[i] = 101325.0f + i * 0.2f;
  assert(analyze<96>(drifting, 96, 4.0f).code == CALIBRATION_TOO_MUCH_DRIFT);

  assert(isnan(relativeAltitudeM(101325.0f, 0.0f)));
  assert(fabsf(relativeAltitudeM(101325.0f, 101325.0f)) < 0.001f);

  puts("GREEN ION V12 calibration tests passed");
  return 0;
}
