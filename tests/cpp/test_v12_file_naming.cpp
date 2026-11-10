#include <cassert>
#include <set>
#include <string>
#include "GreenIonFileNaming.h"

int main() {
  using green_ion::storage::flightSequenceOccupied;
  const std::set<std::string> reserved = {
    "/V12D004.BIN", "/V12D005.SEN", "/V12D006.TXT",
    "/V12D007.JRA", "/V12D008.JRB", "/V12D009.RCV"
  };
  const auto exists = [&](const char *path) {
    return reserved.find(std::string(path)) != reserved.end();
  };
  for (unsigned i = 4; i <= 9; ++i)
    assert(flightSequenceOccupied(i, exists));
  assert(!flightSequenceOccupied(3, exists));
  assert(!flightSequenceOccupied(10, exists));
  assert(flightSequenceOccupied(1000, exists)); // No wrap/reuse after D999.
  return 0;
}
