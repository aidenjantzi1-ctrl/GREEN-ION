#include <cassert>
#include <cstdint>
#include <GreenIonSharedSpi.h>

using green_ion::shared_spi::Coordinator;
using green_ion::shared_spi::OWNER_IDLE;
using green_ion::shared_spi::OWNER_RADIO;
using green_ion::shared_spi::OWNER_SD;
using green_ion::shared_spi::hasDeadlineMargin;

int main() {
  {
    Coordinator bus;
    assert(bus.owner() == OWNER_IDLE);
    assert(bus.beginSd());
    assert(bus.owner() == OWNER_SD);
    assert(bus.beginSd());
    assert(bus.sdDepth() == 2);
    assert(!bus.beginRadio());
    assert(bus.evidence().radioDeferredBusy == 1);
    bus.endSd();
    assert(bus.owner() == OWNER_SD);
    bus.endSd();
    assert(bus.owner() == OWNER_IDLE);
    assert(bus.evidence().sdSections == 1);
    assert(bus.evidence().maxSdDepth == 2);
  }
  {
    Coordinator bus;
    assert(bus.beginRadio());
    assert(bus.owner() == OWNER_RADIO);
    assert(!bus.beginSd());
    assert(bus.evidence().ownershipErrors == 1);
    bus.endRadio();
    assert(bus.owner() == OWNER_IDLE);
    assert(bus.beginSd());
    bus.endSd();
  }
  {
    Coordinator bus;
    bus.endSd();
    assert(bus.evidence().ownershipErrors == 1);
    bus.endRadio();
    assert(bus.evidence().ownershipErrors == 2);
  }
  assert(hasDeadlineMargin(1000, 9000, 5000));
  assert(!hasDeadlineMargin(5000, 8000, 5000));
  assert(!hasDeadlineMargin(9000, 8000, 100));
  // micros() rollover: 0xfffffff0 -> 0x00000020 is 48 us away.
  assert(hasDeadlineMargin(0xfffffff0u, 0x20u, 40));
  assert(!hasDeadlineMargin(0xfffffff0u, 0x20u, 60));
  return 0;
}
