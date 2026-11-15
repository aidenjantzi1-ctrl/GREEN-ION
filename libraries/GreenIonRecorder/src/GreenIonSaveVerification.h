#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "GreenIonRkt4.h"

namespace green_ion {
namespace save_verification {

enum Result : uint8_t {
  SAVE_VALID = 0,
  SAVE_SIZE_MISMATCH,
  SAVE_BAD_RKT4_LAYOUT,
  SAVE_BAD_RKT4_HEADER,
  SAVE_RATE_MISMATCH,
  SAVE_NO_RECORDS,
  SAVE_METADATA_MISSING,
  SAVE_METADATA_INCOMPLETE,
};

inline bool containsBytes(const char *data, size_t length, const char *needle) {
  if (!data || !needle) return false;
  const size_t needleLength = strlen(needle);
  if (!needleLength || needleLength > length) return false;
  for (size_t i = 0; i + needleLength <= length; ++i) {
    if (memcmp(data + i, needle, needleLength) == 0) return true;
  }
  return false;
}

inline Result verifyCompletedPackage(const uint8_t *header,
                                     size_t headerBytes,
                                     size_t fileBytes,
                                     size_t expectedFileBytes,
                                     const char *metadata,
                                     size_t metadataBytes,
                                     const char *logFilename,
                                     uint16_t expectedRateHz) {
  if (fileBytes != expectedFileBytes) return SAVE_SIZE_MISMATCH;
  if (fileBytes < rkt4::kHeaderBytes + rkt4::kRecordBytes ||
      rkt4::trailingByteCount(fileBytes) != 0)
    return SAVE_BAD_RKT4_LAYOUT;
  if (!header || headerBytes != rkt4::kHeaderBytes ||
      rkt4::validateHeader(header, headerBytes) != rkt4::RKT4_VALID)
    return SAVE_BAD_RKT4_HEADER;

  rkt4::Header decoded = {};
  if (!rkt4::decodeHeader(header, headerBytes, decoded))
    return SAVE_BAD_RKT4_HEADER;
  if (decoded.sampleRateHz != expectedRateHz) return SAVE_RATE_MISMATCH;
  if (rkt4::completeRecordCount(fileBytes) == 0) return SAVE_NO_RECORDS;

  if (!metadata || metadataBytes == 0 || !logFilename)
    return SAVE_METADATA_MISSING;
  const char *leaf = logFilename[0] == '/' ? logFilename + 1 : logFilename;
  char fileToken[32] = {};
  char bytesToken[40] = {};
  const int fileTokenBytes =
      snprintf(fileToken, sizeof(fileToken), "file=%s\n", leaf);
  const int bytesTokenBytes =
      snprintf(bytesToken, sizeof(bytesToken), "file_bytes=%lu\n",
               static_cast<unsigned long>(expectedFileBytes));
  if (fileTokenBytes <= 0 ||
      static_cast<size_t>(fileTokenBytes) >= sizeof(fileToken) ||
      bytesTokenBytes <= 0 ||
      static_cast<size_t>(bytesTokenBytes) >= sizeof(bytesToken))
    return SAVE_METADATA_INCOMPLETE;

  if (!containsBytes(metadata, metadataBytes,
                     "GREEN-ION-COMPLETION-1\nstatus=complete\n") ||
      !containsBytes(metadata, metadataBytes, fileToken) ||
      !containsBytes(metadata, metadataBytes, bytesToken))
    return SAVE_METADATA_INCOMPLETE;

  return SAVE_VALID;
}

}  // namespace save_verification
}  // namespace green_ion
