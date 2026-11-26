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
  SAVE_READBACK_UNAVAILABLE,
};

inline const char *resultName(Result result) {
  switch (result) {
    case SAVE_VALID: return "VALID";
    case SAVE_SIZE_MISMATCH: return "SIZE_MISMATCH";
    case SAVE_BAD_RKT4_LAYOUT: return "BAD_RKT4_LAYOUT";
    case SAVE_BAD_RKT4_HEADER: return "BAD_RKT4_HEADER";
    case SAVE_RATE_MISMATCH: return "RATE_MISMATCH";
    case SAVE_NO_RECORDS: return "NO_RECORDS";
    case SAVE_METADATA_MISSING: return "METADATA_MISSING";
    case SAVE_METADATA_INCOMPLETE: return "METADATA_INCOMPLETE";
    case SAVE_READBACK_UNAVAILABLE: return "READBACK_UNAVAILABLE";
    default: return "UNKNOWN";
  }
}

inline const char *findBytes(const char *data, size_t length, const char *needle) {
  if (!data || !needle) return nullptr;
  const size_t needleLength = strlen(needle);
  if (!needleLength || needleLength > length) return nullptr;
  for (size_t i = 0; i + needleLength <= length; ++i) {
    if (memcmp(data + i, needle, needleLength) == 0) return data + i;
  }
  return nullptr;
}

inline bool containsBytes(const char *data, size_t length, const char *needle) {
  return findBytes(data, length, needle) != nullptr;
}

inline size_t countLinesWithPrefix(const char *data,
                                   size_t length,
                                   const char *prefix) {
  if (!data || !prefix) return 0;
  const size_t prefixLength = strlen(prefix);
  size_t count = 0;
  size_t position = 0;
  while (position < length) {
    const size_t lineStart = position;
    while (position < length && data[position] != '\n') ++position;
    const size_t lineLength = position - lineStart;
    if (lineLength >= prefixLength &&
        memcmp(data + lineStart, prefix, prefixLength) == 0)
      ++count;
    if (position < length) ++position;
  }
  return count;
}

inline bool containsExactLine(const char *data,
                              size_t length,
                              const char *expected) {
  if (!data || !expected) return false;
  const size_t expectedLength = strlen(expected);
  size_t position = 0;
  while (position < length) {
    const size_t lineStart = position;
    while (position < length && data[position] != '\n') ++position;
    const size_t lineLength = position - lineStart;
    if (lineLength == expectedLength &&
        memcmp(data + lineStart, expected, expectedLength) == 0)
      return true;
    if (position < length) ++position;
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
  const size_t completeRecords = rkt4::completeRecordCount(fileBytes);
  if (completeRecords == 0) return SAVE_NO_RECORDS;

  if (!metadata || metadataBytes == 0 || !logFilename)
    return SAVE_METADATA_MISSING;

  static const char marker[] = "GREEN-ION-COMPLETION-1\n";
  const char *completionMarker = findBytes(metadata, metadataBytes, marker);
  if (!completionMarker) return SAVE_METADATA_INCOMPLETE;
  const size_t markerOffset =
      static_cast<size_t>(completionMarker - metadata);
  if (markerOffset != 0 && metadata[markerOffset - 1] != '\n')
    return SAVE_METADATA_INCOMPLETE;

  const size_t blockOffset = markerOffset + sizeof(marker) - 1;
  if (blockOffset > metadataBytes) return SAVE_METADATA_INCOMPLETE;
  const char *block = metadata + blockOffset;
  const size_t blockBytes = metadataBytes - blockOffset;
  // A second completion block is ambiguous evidence and must fail closed.
  if (findBytes(block, blockBytes, marker))
    return SAVE_METADATA_INCOMPLETE;

  const char *leaf = logFilename[0] == '/' ? logFilename + 1 : logFilename;
  char fileLine[32] = {};
  char bytesLine[40] = {};
  char recordsLine[40] = {};
  const int fileLineBytes =
      snprintf(fileLine, sizeof(fileLine), "file=%s", leaf);
  const int bytesLineBytes =
      snprintf(bytesLine, sizeof(bytesLine), "file_bytes=%lu",
               static_cast<unsigned long>(expectedFileBytes));
  const int recordsLineBytes =
      snprintf(recordsLine, sizeof(recordsLine), "complete_records=%lu",
               static_cast<unsigned long>(completeRecords));
  if (fileLineBytes <= 0 ||
      static_cast<size_t>(fileLineBytes) >= sizeof(fileLine) ||
      bytesLineBytes <= 0 ||
      static_cast<size_t>(bytesLineBytes) >= sizeof(bytesLine) ||
      recordsLineBytes <= 0 ||
      static_cast<size_t>(recordsLineBytes) >= sizeof(recordsLine))
    return SAVE_METADATA_INCOMPLETE;

  // Validate only the completion block. The start block also contains file=,
  // so whole-file substring matching could otherwise accept a wrong-flight
  // completion record. Required fields must be unique and exact.
  if (countLinesWithPrefix(block, blockBytes, "status=") != 1 ||
      !containsExactLine(block, blockBytes, "status=complete") ||
      countLinesWithPrefix(block, blockBytes, "file=") != 1 ||
      !containsExactLine(block, blockBytes, fileLine) ||
      countLinesWithPrefix(block, blockBytes, "file_bytes=") != 1 ||
      !containsExactLine(block, blockBytes, bytesLine) ||
      countLinesWithPrefix(block, blockBytes, "complete_records=") != 1 ||
      !containsExactLine(block, blockBytes, recordsLine))
    return SAVE_METADATA_INCOMPLETE;

  return SAVE_VALID;
}

}  // namespace save_verification
}  // namespace green_ion
