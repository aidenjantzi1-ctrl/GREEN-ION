#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "GreenIonGpsFreshness.h"

int main() {
  uint32_t fixMs = 0;
  assert(green_ion::gpsFixTimestamp(10000, 0.0, 3000, &fixMs));
  assert(fixMs == 10000);
  assert(green_ion::gpsFixTimestamp(10500, 1.5, 3000, &fixMs));
  assert(fixMs == 9000);  // A later sentence does not move the original fix.
  assert(!green_ion::gpsFixTimestamp(14001, 5.001, 3000, &fixMs));
  assert(!green_ion::gpsFixTimestamp(14001, -1.0, 3000, &fixMs));
  assert(!green_ion::gpsFixTimestamp(14001, NAN, 3000, &fixMs));
  assert(!green_ion::gpsFixTimestamp(14001, INFINITY, 3000, &fixMs));
  assert(!green_ion::gpsFixTimestamp(14001, 0.0, 3000, nullptr));
  assert(green_ion::gpsFixTimestamp(100, 0.2, 3000, &fixMs));
  assert(fixMs == UINT32_MAX - 99);  // millis() rollover remains valid.
  return 0;
}
