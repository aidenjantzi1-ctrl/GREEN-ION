#include <cassert>
#include <cstring>
#include "GreenIonPreflight.h"

int main() {
  using namespace green_ion::preflight;
  Input in = {
    true, true, true, true, false, false, false,
    true, true, true, false, false, false, true, 4050
  };
  Result result = evaluate(in);
  assert(result.flags == 0);
  assert(std::strcmp(result.label, "READY_FOR_REVIEW") == 0);

  in.gpsCurrent = false;
  result = evaluate(in);
  assert(result.flags & GPS_NOT_CURRENT);
  assert(!(result.flags & kBlockingMask));
  assert(std::strcmp(result.label, "CHECK_OPTIONAL") == 0);

  in.batteryMv = 3250;
  result = evaluate(in);
  assert(result.flags & BATTERY_LOW);
  assert(std::strcmp(result.label, "BLOCKED") == 0);

  in.batteryMv = 4050;
  in.tempCleanup = true;
  result = evaluate(in);
  assert(result.flags & TEMP_CLEANUP);
  assert(std::strcmp(result.label, "BLOCKED") == 0);

  in.tempCleanup = false;
  in.maintenance = true;
  result = evaluate(in);
  assert(result.flags & MAINTENANCE_ACTIVE);
  assert(std::strcmp(result.label, "BLOCKED") == 0);

  in.maintenance = false;
  in.calibrated = false;
  in.watcher = false;
  result = evaluate(in);
  assert(result.flags & CALIBRATION_MISSING);
  assert(result.flags & WATCH_NOT_ACTIVE);
  assert(std::strcmp(result.label, "BLOCKED") == 0);

  in.calibrated = true;
  in.watcher = true;
  in.profileSet = false;
  result = evaluate(in);
  assert(result.flags & PROFILE_UNSET);
  assert(std::strcmp(result.label, "CHECK_OPTIONAL") == 0);
  return 0;
}
