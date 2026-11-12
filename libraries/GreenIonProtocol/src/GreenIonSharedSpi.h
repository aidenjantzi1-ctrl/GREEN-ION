#pragma once

#include <stdint.h>

namespace green_ion {
namespace shared_spi {

enum Owner : uint8_t {
  OWNER_IDLE = 0,
  OWNER_SD = 1,
  OWNER_RADIO = 2,
};

struct Evidence {
  uint32_t sdSections = 0;
  uint32_t radioSections = 0;
  uint32_t radioDeferredBusy = 0;
  uint32_t radioDeferredDeadline = 0;
  uint32_t ownershipErrors = 0;
  uint32_t maxSdDepth = 0;
};

class Coordinator {
 public:
  bool beginSd() {
    if (owner_ == OWNER_RADIO) {
      ++evidence_.ownershipErrors;
      return false;
    }
    if (owner_ == OWNER_SD) {
      ++sdDepth_;
      if (sdDepth_ > evidence_.maxSdDepth) evidence_.maxSdDepth = sdDepth_;
      return true;
    }
    owner_ = OWNER_SD;
    sdDepth_ = 1;
    ++evidence_.sdSections;
    if (evidence_.maxSdDepth < 1) evidence_.maxSdDepth = 1;
    return true;
  }

  void endSd() {
    if (owner_ != OWNER_SD || sdDepth_ == 0) {
      ++evidence_.ownershipErrors;
      return;
    }
    if (--sdDepth_ == 0) owner_ = OWNER_IDLE;
  }

  bool beginRadio() {
    if (owner_ != OWNER_IDLE) {
      ++evidence_.radioDeferredBusy;
      return false;
    }
    owner_ = OWNER_RADIO;
    ++evidence_.radioSections;
    return true;
  }

  void endRadio() {
    if (owner_ != OWNER_RADIO) {
      ++evidence_.ownershipErrors;
      return;
    }
    owner_ = OWNER_IDLE;
  }

  void noteRadioDeadlineDeferral() { ++evidence_.radioDeferredDeadline; }

  Owner owner() const { return owner_; }
  bool idle() const { return owner_ == OWNER_IDLE; }
  uint8_t sdDepth() const { return sdDepth_; }
  const Evidence &evidence() const { return evidence_; }

 private:
  Owner owner_ = OWNER_IDLE;
  uint8_t sdDepth_ = 0;
  Evidence evidence_;
};

// Unsigned subtraction is intentional so micros() rollover remains safe.
// A margin with the high bit set means the deadline has already passed.
inline bool hasDeadlineMargin(uint32_t nowUs, uint32_t deadlineUs,
                              uint32_t guardUs) {
  const uint32_t margin = deadlineUs - nowUs;
  return margin < 0x80000000u && margin >= guardUs;
}

}  // namespace shared_spi
}  // namespace green_ion
