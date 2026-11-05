#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "GreenIonBenchTests.h"

int main() {
  using namespace green_ion::bench;

  ImuTest imu(1000, 4);
  assert(imu.valid());
  imu.begin(100, true, true);
  for (uint32_t t = 100; t <= 1100; t += 250)
    imu.observe(t, true, 1.018f, 0.2f, -0.1f, 0.3f, 24.5f);
  assert(imu.result() == RESULT_PASS);
  assert(imu.validSamples() == 5);
  assert(fabsf(imu.meanAccelerationG() - 1.018f) < 0.001f);
  assert(fabsf(imu.gyroBiasZDps() - 0.3f) < 0.001f);
  assert(fabsf(imu.peakAccelerationDeviationG() - 0.018f) < 0.001f);

  imu.begin(1200, true, true);
  imu.observe(1200, true, 1.8f, 0, 0, 0, 24.5f);
  for (uint32_t t = 1450; t <= 2200; t += 250)
    imu.observe(t, true, 0.8f, 0, 0, 0, 24.5f);
  assert(imu.result() == RESULT_INCONCLUSIVE);  // A motion spike cannot average away.

  imu.begin(2300, true, true, 2000, 8);
  assert(imu.durationMs() == 2000);
  for (uint32_t t = 2300; t <= 4300; t += 250)
    imu.observe(t, true, 1.0f, 0, 0, 0, 24.5f);
  assert(imu.result() == RESULT_PASS);

  imu.begin(2000, true, true);
  for (uint32_t t = 2000; t <= 3000; t += 250)
    imu.observe(t, true, 1.0f, 0.0f, 0.0f, 45.0f, 25.0f);
  assert(imu.result() == RESULT_INCONCLUSIVE);
  imu.begin(4000, false, false);
  assert(imu.result() == RESULT_FAIL);
  imu.defer();
  assert(imu.result() == RESULT_DEFERRED);

  GpsTest gps(1000);
  gps.begin(100, true, 10, 8);
  gps.update(1100, 13, 11, false, 0, UINT32_MAX);
  assert(gps.communicationOk());
  assert(gps.result() == RESULT_INCONCLUSIVE);
  assert(gps.nmeaReceived() == 3);
  gps.begin(2000, true, 20, 20);
  gps.update(3000, 22, 22, true, 7, 250);
  assert(gps.result() == RESULT_PASS);
  assert(gps.currentFix());
  gps.begin(UINT32_MAX - 499, true, 30, 30);
  gps.update(600, 31, 31, false, 0, UINT32_MAX);
  assert(gps.result() == RESULT_INCONCLUSIVE);
  gps.begin(4000, true, 40, 40);
  gps.update(5000, 40, 40, false, 0, UINT32_MAX);
  assert(gps.result() == RESULT_FAIL);
  return 0;
}
