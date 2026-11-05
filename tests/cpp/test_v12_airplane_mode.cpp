#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "GreenIonAirplaneMode.h"

using namespace green_ion::airplane;

int main() {
  Controller mode(10000u);
  const Readiness ready = {true, true, true, 3900, 3500};
  assert(checkStart(AIRPLANE_DISABLED, ready) == START_NOT_READY);
  assert(mode.prepare());
  mode.calibrationReady(true);
  assert(mode.state() == AIRPLANE_READY);
  assert(mode.start(0xfffffff0u, ready) == START_ALLOWED);
  assert(mode.radioSilenceLatched());
  assert(!mode.automaticStopDue(0x00001000u));
  assert(mode.automaticStopDue(0x00002710u));
  assert(mode.stop() && mode.state() == AIRPLANE_COMPLETE);
  assert(mode.radioSilenceLatched());
  assert(mode.clearRadioSilenceWhenIdle());

  Controller guards(1000u);
  assert(guards.prepare());
  guards.calibrationReady(true);
  Readiness bad = ready;
  bad.sdOk = false;
  assert(guards.start(0, bad) == START_SD_UNAVAILABLE);
  bad = ready;
  bad.barometerOk = false;
  assert(guards.start(0, bad) == START_BAROMETER_UNAVAILABLE);
  bad = ready;
  bad.imuOk = false;
  assert(guards.start(0, bad) == START_IMU_UNAVAILABLE);
  bad = ready;
  bad.batteryMv = 3400;
  assert(guards.start(0, bad) == START_BATTERY_LOW);

  const float baseline = 101325.0f;
  assert(fabsf(cabinPressureAltitudeDeltaM(baseline, baseline)) < 0.001f);
  assert(cabinPressureAltitudeDeltaM(90000.0f, baseline) > 900.0f);
  assert(isnan(cabinPressureAltitudeDeltaM(NAN, baseline)));
  assert(isnan(cabinPressureAltitudeDeltaM(90000.0f, 0.0f)));

  Controller restored(1000u);
  restored.restoreRadioSilence();
  assert(restored.state() == AIRPLANE_RADIO_SILENCE_LOCKED);
  assert(restored.radioSilenceLatched());
  return 0;
}
