#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>

namespace green_ion {
namespace calibration {

enum ResultCode : uint8_t {
  CALIBRATION_VALID = 0,
  CALIBRATION_TOO_FEW_SAMPLES,
  CALIBRATION_BAD_DURATION,
  CALIBRATION_INVALID_SAMPLE,
  CALIBRATION_TOO_NOISY,
  CALIBRATION_TOO_MUCH_DRIFT,
};

struct Result {
  ResultCode code;
  float baselinePressurePa;
  float medianPressurePa;
  float noisePressurePa;
  float noiseAltitudeM;
  float driftPaPerSecond;
};

inline float relativeAltitudeM(float pressurePa, float baselinePressurePa) {
  if (!isfinite(pressurePa) || !isfinite(baselinePressurePa) || baselinePressurePa <= 0)
    return NAN;
  return 44330.0f * (1.0f - powf(pressurePa / baselinePressurePa, 0.190294957f));
}

// V11-compatible stationary analysis with fixed storage and no heap use.
template <size_t Capacity>
Result analyze(const float *chronologicalPressurePa, size_t count,
               float collectionSeconds) {
  Result result = {CALIBRATION_TOO_FEW_SAMPLES, NAN, NAN, NAN, NAN, NAN};
  if (count < 80 || count > Capacity) return result;
  if (!isfinite(collectionSeconds) || collectionSeconds <= 0) {
    result.code = CALIBRATION_BAD_DURATION;
    return result;
  }

  float ordered[Capacity];
  for (size_t i = 0; i < count; ++i) {
    const float value = chronologicalPressurePa[i];
    if (!isfinite(value) || value < 30000.0f || value > 120000.0f) {
      result.code = CALIBRATION_INVALID_SAMPLE;
      return result;
    }
    ordered[i] = value;
  }

  size_t driftWindow = count / 4;
  if (driftWindow > 12) driftWindow = 12;
  double firstSum = 0;
  double lastSum = 0;
  for (size_t i = 0; i < driftWindow; ++i) {
    firstSum += chronologicalPressurePa[i];
    lastSum += chronologicalPressurePa[count - driftWindow + i];
  }
  result.driftPaPerSecond = static_cast<float>(
      ((lastSum - firstSum) / driftWindow) / collectionSeconds);

  for (size_t i = 1; i < count; ++i) {
    const float value = ordered[i];
    size_t j = i;
    while (j > 0 && ordered[j - 1] > value) {
      ordered[j] = ordered[j - 1];
      --j;
    }
    ordered[j] = value;
  }

  result.medianPressurePa = ordered[count / 2];
  size_t trim = count / 10;
  if (trim < 4) trim = 4;
  const size_t first = trim;
  const size_t last = count - trim;
  double sum = 0;
  for (size_t i = first; i < last; ++i) sum += ordered[i];
  result.baselinePressurePa = static_cast<float>(sum / (last - first));

  double squareSum = 0;
  for (size_t i = first; i < last; ++i) {
    const double delta = ordered[i] - result.baselinePressurePa;
    squareSum += delta * delta;
  }
  result.noisePressurePa = static_cast<float>(
      sqrt(squareSum / ((last - first) > 1 ? (last - first - 1) : 1)));
  result.noiseAltitudeM = fabsf(relativeAltitudeM(
      result.baselinePressurePa + result.noisePressurePa,
      result.baselinePressurePa));

  if (!isfinite(result.noiseAltitudeM) || result.noiseAltitudeM > 0.55f) {
    result.code = CALIBRATION_TOO_NOISY;
    return result;
  }
  if (!isfinite(result.driftPaPerSecond) || fabsf(result.driftPaPerSecond) > 1.5f) {
    result.code = CALIBRATION_TOO_MUCH_DRIFT;
    return result;
  }
  result.code = CALIBRATION_VALID;
  return result;
}

}  // namespace calibration
}  // namespace green_ion
