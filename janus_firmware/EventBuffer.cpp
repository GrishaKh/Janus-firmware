#include "EventBuffer.h"

#include "Config.h"

void RamRingLogger::record(const String& eventId, const String& json) {
  if (_buf.size() >= EVENT_RING_CAPACITY) {
    Serial.print(F("[ring] capacity reached, evicting oldest event_id="));
    Serial.println(_buf.front().eventId);
    _buf.erase(_buf.begin());
  }
  _buf.push_back({eventId, json});
}

void RamRingLogger::peekBatch(std::vector<PendingEvent>& out, size_t maxN) const {
  out.clear();
  if (_buf.empty() || maxN == 0) return;
  size_t take = _buf.size() < maxN ? _buf.size() : maxN;
  out.reserve(take);
  for (size_t i = 0; i < take; ++i) out.push_back(_buf[i]);
}

void RamRingLogger::drop(const std::vector<String>& eventIds) {
  for (const String& id : eventIds) dropOne(id);
}

void RamRingLogger::dropOne(const String& eventId) {
  for (auto it = _buf.begin(); it != _buf.end(); ++it) {
    if (it->eventId == eventId) { _buf.erase(it); return; }
  }
}
