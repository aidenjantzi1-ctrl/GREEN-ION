#include <cassert>
#include "GreenIonFlightReadiness.h"

using green_ion::readiness::Input;
using green_ion::readiness::Result;

static Input allGood() {
  return {true,true,true,3900,true,true,true,true,true,true,
          true,true,true,true,true};
}

int main() {
  auto input=allGood();
  Result r=green_ion::readiness::evaluate(input);
  assert(r.ready);
  assert(r.criticalFlags==0);
  assert(r.warningFlags==0);

  // Radio and GPS are explicitly optional: local recorder readiness remains true.
  input=allGood(); input.radio=false;
  r=green_ion::readiness::evaluate(input);
  assert(r.ready);
  assert(r.warningFlags & green_ion::readiness::WARNING_LORA);

  input=allGood(); input.gps=false; input.radio=false;
  r=green_ion::readiness::evaluate(input);
  assert(r.ready);
  assert(r.warningFlags & green_ion::readiness::WARNING_GPS);
  assert(r.warningFlags & green_ion::readiness::WARNING_LORA);

  // Every local-recording dependency is blocking.
  input=allGood(); input.sd=false;
  assert(!green_ion::readiness::evaluate(input).ready);
  input=allGood(); input.barometer=false;
  assert(!green_ion::readiness::evaluate(input).ready);
  input=allGood(); input.imu=false;
  assert(!green_ion::readiness::evaluate(input).ready);
  input=allGood(); input.batteryMv=3599;
  assert(!green_ion::readiness::evaluate(input).ready);
  input=allGood(); input.sdIntegrity=false;
  assert(!green_ion::readiness::evaluate(input).ready);
  input=allGood(); input.calibrated=false;
  assert(!green_ion::readiness::evaluate(input).ready);
  input=allGood(); input.launchSimulation=false;
  assert(!green_ion::readiness::evaluate(input).ready);
  input=allGood(); input.watcherActive=false;
  assert(!green_ion::readiness::evaluate(input).ready);

  // Secondary log trouble is advisory because the RKT4 BIN remains authoritative.
  input=allGood(); input.secondaryLogHealthy=false;
  r=green_ion::readiness::evaluate(input);
  assert(r.ready);
  assert(r.warningFlags & green_ion::readiness::WARNING_SECONDARY_LOG);
}
