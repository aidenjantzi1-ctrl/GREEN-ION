#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "GreenIonRkt4.h"

namespace green_ion {
namespace recovery {

static const uint16_t kCheckpointVersion = 1;
static const size_t kCheckpointBytes = 32;

struct Checkpoint {
  uint32_t generation;
  uint32_t committedBytes;
  uint32_t recordCount;
  uint32_t elapsedMs;
};

enum CheckpointResult : uint8_t {
  CHECKPOINT_VALID = 0,
  CHECKPOINT_SHORT,
  CHECKPOINT_BAD_MAGIC,
  CHECKPOINT_BAD_VERSION,
  CHECKPOINT_BAD_SIZE,
  CHECKPOINT_BAD_CRC,
  CHECKPOINT_BAD_ALIGNMENT,
  CHECKPOINT_BAD_RECORD_COUNT,
  CHECKPOINT_BEYOND_FILE,
};

struct Rkt4Inspection {
  rkt4::ValidationResult headerResult;
  uint32_t completeRecords;
  uint8_t trailingBytes;
};

enum RecoveryCommitResult : uint8_t {
  RECOVERY_COMMITTED = 0,
  RECOVERY_NO_JOURNAL,
  RECOVERY_REPORT_WRITE_FAILED,
  RECOVERY_SLOT_A_REMOVE_FAILED,
  RECOVERY_SLOT_B_REMOVE_FAILED,
};

class RecoveryCommitter {
 public:
  virtual ~RecoveryCommitter() {}
  virtual bool saveReport() = 0;
  virtual bool removeSlotA() = 0;
  virtual bool removeSlotB() = 0;
};

// The report is durable before journal cleanup begins. Cleanup stops at the
// first failure, preserving the remaining journal as boot-visible evidence.
inline RecoveryCommitResult commitRecovery(RecoveryCommitter &committer,
                                           bool haveSlotA, bool haveSlotB) {
  if (!haveSlotA && !haveSlotB) return RECOVERY_NO_JOURNAL;
  if (!committer.saveReport()) return RECOVERY_REPORT_WRITE_FAILED;
  if (haveSlotA && !committer.removeSlotA())
    return RECOVERY_SLOT_A_REMOVE_FAILED;
  if (haveSlotB && !committer.removeSlotB())
    return RECOVERY_SLOT_B_REMOVE_FAILED;
  return RECOVERY_COMMITTED;
}

inline uint32_t checkpointCrc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffffu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

inline bool checkpointFieldsValid(const Checkpoint &checkpoint) {
  if (checkpoint.committedBytes < rkt4::kHeaderBytes) return false;
  const uint32_t payloadBytes = checkpoint.committedBytes - rkt4::kHeaderBytes;
  return payloadBytes % rkt4::kRecordBytes == 0 &&
         checkpoint.recordCount == payloadBytes / rkt4::kRecordBytes;
}

inline bool encodeCheckpoint(uint8_t out[kCheckpointBytes],
                             const Checkpoint &checkpoint) {
  if (!checkpointFieldsValid(checkpoint)) return false;
  memset(out, 0, kCheckpointBytes);
  memcpy(out, "GIC1", 4);
  rkt4::writeU16(out + 4, kCheckpointVersion);
  rkt4::writeU16(out + 6, static_cast<uint16_t>(kCheckpointBytes));
  rkt4::writeU32(out + 8, checkpoint.generation);
  rkt4::writeU32(out + 12, checkpoint.committedBytes);
  rkt4::writeU32(out + 16, checkpoint.recordCount);
  rkt4::writeU32(out + 20, checkpoint.elapsedMs);
  rkt4::writeU32(out + 28, checkpointCrc32(out, 28));
  return true;
}

inline CheckpointResult decodeCheckpoint(const uint8_t *data, size_t length,
                                         Checkpoint &out) {
  if (length < kCheckpointBytes) return CHECKPOINT_SHORT;
  if (memcmp(data, "GIC1", 4) != 0) return CHECKPOINT_BAD_MAGIC;
  if (rkt4::readU16(data + 4) != kCheckpointVersion)
    return CHECKPOINT_BAD_VERSION;
  if (rkt4::readU16(data + 6) != kCheckpointBytes)
    return CHECKPOINT_BAD_SIZE;
  if (rkt4::readU32(data + 28) != checkpointCrc32(data, 28))
    return CHECKPOINT_BAD_CRC;

  Checkpoint candidate = {
    rkt4::readU32(data + 8),
    rkt4::readU32(data + 12),
    rkt4::readU32(data + 16),
    rkt4::readU32(data + 20),
  };
  if (candidate.committedBytes < rkt4::kHeaderBytes ||
      (candidate.committedBytes - rkt4::kHeaderBytes) % rkt4::kRecordBytes != 0)
    return CHECKPOINT_BAD_ALIGNMENT;
  if (candidate.recordCount !=
      (candidate.committedBytes - rkt4::kHeaderBytes) / rkt4::kRecordBytes)
    return CHECKPOINT_BAD_RECORD_COUNT;
  out = candidate;
  return CHECKPOINT_VALID;
}

inline bool generationIsNewer(uint32_t candidate, uint32_t current) {
  const uint32_t distance = candidate - current;
  return distance != 0 && distance < 0x80000000u;
}

inline CheckpointResult checkpointForFile(const uint8_t *data, size_t length,
                                          uint32_t fileBytes, Checkpoint &out) {
  CheckpointResult result = decodeCheckpoint(data, length, out);
  if (result != CHECKPOINT_VALID) return result;
  return out.committedBytes <= fileBytes ? CHECKPOINT_VALID : CHECKPOINT_BEYOND_FILE;
}

inline bool selectNewestCheckpoint(const uint8_t *slotA, size_t slotABytes,
                                   const uint8_t *slotB, size_t slotBBytes,
                                   uint32_t fileBytes, Checkpoint &out) {
  Checkpoint a = {};
  Checkpoint b = {};
  const bool validA = checkpointForFile(slotA, slotABytes, fileBytes, a) ==
                      CHECKPOINT_VALID;
  const bool validB = checkpointForFile(slotB, slotBBytes, fileBytes, b) ==
                      CHECKPOINT_VALID;
  if (!validA && !validB) return false;
  out = !validB || (validA && generationIsNewer(a.generation, b.generation)) ? a : b;
  return true;
}

inline Rkt4Inspection inspectRkt4(const uint8_t *header, size_t headerBytes,
                                  uint32_t fileBytes) {
  Rkt4Inspection result = {rkt4::validateHeader(header, headerBytes), 0, 0};
  if (result.headerResult != rkt4::RKT4_VALID) return result;
  result.completeRecords = static_cast<uint32_t>(rkt4::completeRecordCount(fileBytes));
  result.trailingBytes = static_cast<uint8_t>(rkt4::trailingByteCount(fileBytes));
  return result;
}

}  // namespace recovery
}  // namespace green_ion
