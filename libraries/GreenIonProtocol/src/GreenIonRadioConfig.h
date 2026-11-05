#pragma once

#include <stdint.h>

namespace green_ion {
namespace radio_config {

// Provisional Lucerne Valley development settings. Coordinate the final
// frequency with ROC before field use; these constants are not a compliance
// determination or a guarantee that the channel is clear.
static constexpr float kFrequencyMhz = 920.5f;
static const long kSignalBandwidthHz = 125000L;
static const uint8_t kSpreadingFactor = 7;
static const uint8_t kCodingRateDenominator = 5;
static const int8_t kTxPowerDbm = 13;

}  // namespace radio_config
}  // namespace green_ion
