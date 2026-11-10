#pragma once
// Preserve flight identity if ANY sidecar/journal/report already exists.
// Works with host-mock file systems as well as ESP32 SD.exists().
#include <stdint.h>
#include <stdio.h>
namespace green_ion {
namespace storage {
static constexpr unsigned kSequenceLimit = 1000;
static constexpr const char *kReservedSuffixes[] =
    {".BIN", ".SEN", ".TXT", ".JRA", ".JRB", ".RCV"};
template <typename Exists>
inline bool flightSequenceOccupied(unsigned sequence, Exists exists) {
  if (sequence >= kSequenceLimit) return true;
  for (const char *suffix : kReservedSuffixes) {
    char path[14] = {};
    const int n = snprintf(path, sizeof(path), "/V12D%03u%s", sequence, suffix);
    if (n < 0 || static_cast<size_t>(n) >= sizeof(path) || exists(path))
      return true;
  }
  return false;
}
}  // namespace storage
}  // namespace green_ion
