#include <assert.h>
#include <stdio.h>

#include "GreenIonTelemetryQueue.h"

static green_ion::TelemetryPacket packet(uint32_t sequence) {
  green_ion::TelemetryPacket value;
  green_ion::initialize(value);
  value.sequence = sequence;
  green_ion::seal(value);
  return value;
}

int main() {
  green_ion::TelemetryQueue<3> queue;
  assert(queue.size() == 0 && queue.front() == NULL && !queue.pop());
  queue.push(packet(1));
  queue.push(packet(2));
  queue.push(packet(3));
  assert(queue.size() == 3 && queue.front()->sequence == 1);
  queue.push(packet(4));
  assert(queue.size() == 3 && queue.front()->sequence == 2);
  assert(queue.dropped() == 1);
  assert(queue.pop() && queue.front()->sequence == 3);
  assert(queue.pop() && queue.front()->sequence == 4);
  assert(queue.pop() && queue.front() == NULL);

  using green_ion::radioDispatchAllowed;
  assert(radioDispatchAllowed(true, false, false, true, 1000, 7000, 5000));
  assert(!radioDispatchAllowed(false, false, false, true, 1000, 7000, 5000));
  assert(!radioDispatchAllowed(true, true, false, true, 1000, 7000, 5000));
  assert(!radioDispatchAllowed(true, false, true, true, 1000, 7000, 5000));
  assert(!radioDispatchAllowed(true, false, false, false, 1000, 7000, 5000));
  assert(!radioDispatchAllowed(true, false, false, true, 3000, 7000, 5000));
  assert(!radioDispatchAllowed(true, false, false, true, 7001, 7000, 1));
  const uint32_t nearWrap = 0xfffffff0u;
  assert(radioDispatchAllowed(true, false, false, true, nearWrap,
                              nearWrap + 6000u, 5000));
  puts("GREEN ION V12 telemetry-queue tests passed");
  return 0;
}
