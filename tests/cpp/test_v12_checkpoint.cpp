#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "GreenIonCheckpoint.h"

using namespace green_ion::recovery;

int main() {
  const Checkpoint first = {1, 32 + 10 * 16, 10, 2000};
  const Checkpoint second = {2, 32 + 20 * 16, 20, 4000};
  uint8_t slotA[kCheckpointBytes];
  uint8_t slotB[kCheckpointBytes];
  assert(encodeCheckpoint(slotA, first));
  assert(encodeCheckpoint(slotB, second));
  assert(memcmp(slotA, "GIC1", 4) == 0);
  const uint8_t firstGoldenCrc[4] = {0x1c, 0xa1, 0x47, 0xab};
  assert(memcmp(slotA + 28, firstGoldenCrc, sizeof(firstGoldenCrc)) == 0);

  Checkpoint decoded = {};
  assert(decodeCheckpoint(slotA, sizeof(slotA), decoded) == CHECKPOINT_VALID);
  assert(decoded.generation == first.generation);
  assert(decoded.committedBytes == first.committedBytes);
  assert(decoded.recordCount == first.recordCount);
  assert(decoded.elapsedMs == first.elapsedMs);

  Checkpoint selected = {};
  assert(selectNewestCheckpoint(slotA, sizeof(slotA), slotB, sizeof(slotB),
                                second.committedBytes, selected));
  assert(selected.generation == 2);

  uint8_t corruptB[kCheckpointBytes];
  memcpy(corruptB, slotB, sizeof(corruptB));
  corruptB[12] ^= 0x40;
  assert(decodeCheckpoint(corruptB, sizeof(corruptB), decoded) == CHECKPOINT_BAD_CRC);
  assert(selectNewestCheckpoint(slotA, sizeof(slotA), corruptB, sizeof(corruptB),
                                second.committedBytes, selected));
  assert(selected.generation == 1);

  assert(checkpointForFile(slotB, sizeof(slotB), first.committedBytes, decoded) ==
         CHECKPOINT_BEYOND_FILE);
  assert(!encodeCheckpoint(slotA, Checkpoint{3, 33, 0, 0}));
  assert(!encodeCheckpoint(slotA, Checkpoint{3, 48, 2, 0}));

  assert(generationIsNewer(0, 0xffffffffu));
  assert(!generationIsNewer(0xffffffffu, 0));
  assert(!generationIsNewer(0x80000000u, 0));

  uint8_t header[green_ion::rkt4::kHeaderBytes];
  assert(green_ion::rkt4::encodeHeader(
      header, green_ion::rkt4::Header{50, 101325.0f, 4000}));
  Rkt4Inspection clean = inspectRkt4(header, sizeof(header), 32 + 3 * 16);
  assert(clean.headerResult == green_ion::rkt4::RKT4_VALID);
  assert(clean.completeRecords == 3);
  assert(clean.trailingBytes == 0);
  Rkt4Inspection torn = inspectRkt4(header, sizeof(header), 32 + 3 * 16 + 7);
  assert(torn.completeRecords == 3);
  assert(torn.trailingBytes == 7);
  header[0] = 'X';
  assert(inspectRkt4(header, sizeof(header), 96).headerResult ==
         green_ion::rkt4::RKT4_BAD_MAGIC);

  puts("GREEN ION V12 checkpoint/recovery tests passed");
  return 0;
}
