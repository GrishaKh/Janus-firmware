// EventBuffer.h — RamRingLogger, the v1 in-memory event buffer.
//
// Sized to EVENT_RING_CAPACITY (64) — about 16 KB at typical event sizes,
// enough for ~30 minutes of moderate offline use. When the ring fills,
// push() evicts the oldest entry and logs a warning.
#pragma once

#include <Arduino.h>
#include <vector>

#include "Logger.h"

struct PendingEvent {
  String eventId;
  String json;
};

class RamRingLogger : public Logger {
 public:
  // Logger interface.
  void   record(const String& eventId, const String& json) override;
  size_t pendingCount() const override { return _buf.size(); }

  // RamRingLogger-specific drain helpers.
  void peekBatch(std::vector<PendingEvent>& out, size_t maxN) const;
  void drop(const std::vector<String>& eventIds);   // bulk
  void dropOne(const String& eventId);              // single, fast-path

 private:
  std::vector<PendingEvent> _buf;
};
